# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 Advanced Micro Devices, Inc.

import argparse
import re

import graphviz
import networkx as nx

from ...schema.protobuf.et_def_pb2 import GlobalMetadata, Node
from ..third_party.utils.protolib import decodeMessage as decode_message
from ..third_party.utils.protolib import openFileRd as open_file_rd


# TODO can add escape_label for the name to get rid of chars
def escape_label(label: str) -> str:
    """
    Escapes special characters in labels for graph rendering.

    Args:
        label (str): The original label string.

    Returns:
        str: The escaped label string.
    """
    # Define special characters to escape
    special_chars = "{}()<>\\[\\]|&-"
    # Escape special characters
    return re.sub(f"([{special_chars}])", r"\\\1", label)


def render_graphviz(node, f):
    if node.type == 4:
        node_name = node.name + "\\n" + str(node.duration_micros)
        color = "seagreen2"
    else:
        node_name = f"{node.name}"
        color = "lightskyblue"
    f.node(name=f"{node.id}", label=node_name, id=str(node.id), shape="record", style="filled", fillcolor=color)

    for data_dep_id in node.data_deps:
        f.edge(str(data_dep_id), str(node.id), arrowhead="normal")

    for ctrl_dep_id in node.ctrl_deps:
        f.edge(str(ctrl_dep_id), str(node.id), arrowhead="tee")


def render_graphml(node, G):
    G.add_node(node.id, label=node.name)
    for data_dep_id in node.data_deps:
        G.add_edge(data_dep_id, node.id, dependency="data")
    for ctrl_dep_id in node.ctrl_deps:
        G.add_edge(ctrl_dep_id, node.id, dependency="control")


def process_nodes(decode_message, et, gm, render_func, graph_obj):
    node = Node()
    decode_message(et, gm)
    while decode_message(et, node):
        render_func(node, graph_obj)


def main() -> None:
    """Generate an output graph file in the specified format (PDF, DOT, or GraphML)."""
    parser = argparse.ArgumentParser(description="Execution Trace Visualizer")
    parser.add_argument("--input_filename", type=str, required=True, help="Input Chakra execution trace filename")
    parser.add_argument(
        "--output_filename",
        type=str,
        required=True,
        help=(
            "Output graph filename. Supported extensions are pdf, dot, and graphml. "
            "Recommend using graphml for large graphs for rendering speed."
        ),
    )
    args = parser.parse_args()
    et = open_file_rd(args.input_filename)
    gm = GlobalMetadata()

    if args.output_filename.endswith((".pdf", ".dot")):
        f = graphviz.Digraph()
        process_nodes(decode_message, et, gm, render_graphviz, f)
        output_format = "pdf" if args.output_filename.endswith(".pdf") else "dot"
        f.render(args.output_filename.replace(f".{output_format}", ""), format=output_format, cleanup=True)
    elif args.output_filename.endswith(".graphml"):
        G = nx.DiGraph()
        process_nodes(decode_message, et, gm, render_graphml, G)
        nx.write_graphml(G, args.output_filename)
    else:
        print("Unknown output file extension. Must be one of pdf, dot, graphml.")

    et.close()


if __name__ == "__main__":
    main()
