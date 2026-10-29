# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

import argparse
import json
import logging
import sys
from enum import IntEnum
from logging import FileHandler
from typing import Any, Dict, List, Tuple

from ...schema.protobuf.et_def_pb2 import (
    COMM_COLL_NODE,
    COMM_RECV_NODE,
    COMM_SEND_NODE,
    COMP_NODE,
    MEM_LOAD_NODE,
    MEM_STORE_NODE,
)


class TID(IntEnum):
    """Enum to store the type of node."""

    MEMORY = 1
    COMP = 2
    COMM = 3


# Define a mapping of node types for easier lookup
NODE_TYPE_MAP = {
    MEM_LOAD_NODE: TID.MEMORY,
    MEM_STORE_NODE: TID.MEMORY,
    COMP_NODE: TID.COMP,
    COMM_SEND_NODE: TID.COMM,
    COMM_RECV_NODE: TID.COMM,
    COMM_COLL_NODE: TID.COMM,
}


def get_logger(log_filename: str) -> logging.Logger:
    """
    Initialize and configure a logger with a file handler and a stream handler.

    Args:
        log_filename (str): The name of the log file.

    Returns:
        logging.Logger: The configured logger.
    """
    formatter = logging.Formatter("%(levelname)s [%(asctime)s] %(message)s", datefmt="%m/%d/%Y %I:%M:%S %p")

    file_handler = FileHandler(log_filename, mode="w")
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(formatter)

    stream_handler = logging.StreamHandler()
    stream_handler.setLevel(logging.WARNING)
    stream_handler.setFormatter(formatter)

    logger = logging.getLogger(__file__)
    logger.setLevel(logging.DEBUG)
    logger.addHandler(file_handler)
    logger.addHandler(stream_handler)

    return logger


def get_tid_and_append(node_name: str, node_type: int) -> "tuple[TID, str]":
    """
    Determine the TID (Thread ID) based on the node type and append it to the node name.

    Args:
        node_name (str): The name of the node.
        node_type (int): The type of the node.

    Returns:
        TID: The Thread ID.
    """
    tid = NODE_TYPE_MAP.get(node_type)
    if tid is not None:
        return tid, f"{node_name}_{tid.name.lower()}"
    else:
        raise ValueError(f"Node type cannot be identified from {node_name}")


def parse_event(line: str) -> Tuple[str, int, int, int, str, int]:
    """
    Parse a line of event data and extract relevant information.

    Args:
        line (str): The event data in the form of a string.

    Returns:
        Tuple[str, int, int, int, str, int]: A tuple containing parsed event information.
    """
    try:
        cols = line.strip().split(",")
        trace_type = cols[0]
        npu_id = int(cols[1].split("=")[1])
        curr_cycle = int(cols[2].split("=")[1])
        node_id = int(cols[3].split("=")[1])
        node_name = cols[4].split("=")[1]
        node_type = int(cols[5].split("=")[1])
        return (trace_type, npu_id, curr_cycle, node_id, node_name, node_type)
    except Exception as e:
        raise ValueError(f'Cannot parse the following event -- "{line}": {e}') from e


def get_trace_events(input_filename: str, num_npus: int, npu_frequency: int) -> List[Dict[str, Any]]:
    """
    Process the input file to generate trace events.

    Args:
        input_filename (str): The name of the input timeline file.
        num_npus (int): Number of NPUs in the system.
        npu_frequency (int): NPU frequency in MHz.

    Returns:
        List[Dict[str, Any]]: A list of trace events as dictionaries.
    """
    trace_dict = {i: {} for i in range(num_npus)}
    trace_events = []

    # Initialize function to set process and thread names
    for n_npu in range(num_npus):
        for tid in TID:
            trace_events.append(
                {"args": {"name": tid.name}, "name": "thread_name", "ph": "M", "pid": n_npu, "tid": tid, "ts": 0}
            )
        trace_events.append(
            {"args": {"name": "GPU"}, "name": "process_name", "ph": "M", "pid": n_npu, "tid": 0, "ts": 0}
        )

    # Cycle through input file to add events
    with open(input_filename, "r") as f:
        for line in f:
            if "issue" in line or "callback" in line:
                (trace_type, npu_id, curr_cycle, node_id, node_name, node_type) = parse_event(line)
                if trace_type == "issue":
                    trace_dict[npu_id].update({node_id: [node_name, curr_cycle]})
                elif trace_type == "callback":
                    node_name = trace_dict[npu_id][node_id][0]
                    tid, node_name = get_tid_and_append(node_name, node_type)
                    issued_cycle = trace_dict[npu_id][node_id][1]
                    issued_ms = (issued_cycle / npu_frequency) / 1_000
                    duration_in_cycles = curr_cycle - issued_cycle
                    duration_in_ms = duration_in_cycles / (npu_frequency * 1_000)

                    trace_events.append(
                        {
                            "pid": npu_id,
                            "tid": tid,
                            "ts": issued_ms,
                            "dur": duration_in_ms,
                            "ph": "X",
                            "name": node_name,
                            "args": {"ms": duration_in_ms},
                        }
                    )

                    del trace_dict[npu_id][node_id]
                else:
                    raise ValueError(f"Unsupported trace_type, {trace_type}")

    return trace_events


def write_trace_events(output_filename: str, num_npus: int, trace_events: List[Dict[str, Any]]) -> None:
    """
    Write the generated trace events to an output file.

    Args:
        output_filename (str): The name of the output trace file.
        num_npus (int): Number of NPUs in the system.
        trace_events (List[Dict[str, Any]]): List of trace events as dictionaries.
    """
    output_dict = {"meta_user": "aras", "traceEvents": trace_events, "meta_cpu_count": num_npus}
    with open(output_filename, "w") as f:
        json.dump(output_dict, f)


def main() -> None:
    """Parse the command line arguments and begin operation."""
    parser = argparse.ArgumentParser(description="Timeline Visualizer")
    parser.add_argument("--input_filename", type=str, required=True, help="Input timeline filename")
    parser.add_argument("--output_filename", type=str, required=True, help="Output trace filename")
    parser.add_argument("--num_npus", type=int, required=True, help="Number of NPUs in a system")
    parser.add_argument("--npu_frequency", type=int, required=True, help="NPU frequency in MHz")
    parser.add_argument("--log_filename", type=str, default="debug.log", help="Log filename")
    args = parser.parse_args()

    logger = get_logger(args.log_filename)
    logger.debug(" ".join(sys.argv))

    try:
        trace_events = get_trace_events(args.input_filename, args.num_npus, args.npu_frequency)
        write_trace_events(args.output_filename, args.num_npus, trace_events)
    except Exception as e:
        logger.error(str(e))
        sys.exit(1)


if __name__ == "__main__":
    main()
