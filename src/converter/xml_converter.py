# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2023 Advanced Micro Devices, Inc.
# Base source code from Jinsun Yoo <jinsun@gatech.edu>

from io import BufferedWriter
from typing import Dict, Tuple
from xml.etree import ElementTree

from ...schema.protobuf.et_def_pb2 import (
    COMM_RECV_NODE,
    COMM_SEND_NODE,
    COMP_NODE,
    GlobalMetadata,
    Node,
)
from ...schema.protobuf.et_def_pb2 import AttributeProto as ChakraAttr
from ..third_party.utils.protolib import encodeMessage


class XmlStep:
    """
    Base class of each XML step.

    XmlStep is a base class that encapsulates each step in the custom collective XML representation.
    Each XmlStep can have either one Chakra (if not fused) or more (if fused) Chakra xml_steps.

    When fused, add_parent() and encode_message() methods must be overriden accordingly.
    """

    def __init__(self, node_id: int) -> None:
        """
        Initialize the base class.

        Args:
            node_id (int): chakra id of the new node
        """
        self._node = Node()
        self._node.id = node_id

    def _get_first_node(self) -> Node:
        """
        Return the first Chakra node in each XML step.

        Returns
            Node: very first Chakra node to run in the XML step
        """
        return self._node

    def _get_last_node(self) -> Node:
        """
        Return the last Chakra node in each XML step.

        Returns
            Node: very last Chakra node to run in the XML step
        """
        return self._node

    def add_parent(self, parent: "XmlStep") -> None:
        """
        Add a parent step to this XmlStep.

        Args:
            parent (XmlStep): parent step to this XmlStep.

        Raises:
            Exception: if the given parent is Nop
            Exception: if this step is Nop
        """
        # NOPs: raise Exception
        if type(parent) is XmlNopStep:
            raise Exception("Nop parent added as parent")
        if type(self) is XmlNopStep:
            raise Exception("Trying to add parent to Nop")

        # assign dependency between (last node of parent) -- (first node of this)
        parent_last_node = parent._get_last_node()
        self_first_node = self._get_first_node()
        self_first_node.data_deps.append(parent_last_node.id)

    def encode_message(self, g: BufferedWriter) -> None:
        """
        Encode Chakra node into the protobuf file.

        Args:
            g (BufferedWriter): protobuf file to dump chakra node
        """
        encodeMessage(g, self._node)

    @staticmethod
    def _calculate_reduction_time(data_size: int) -> int:
        """
        Calculate the chunk reduction time.

        Args:
            data_size (int): chunk size to reduce (in bytes)

        Returns:
            int: estimated reduction time (in us).
        """
        # FIXME: better reduction time modeling is required.

        # model only memory access time
        LOCAL_MEM_BW = 50  # GB/s = B/ns

        # memory access size: 3 * data_size
        # (2 for reading two chunks + 1 for writing the resulting chunk)
        rw_size = 3 * data_size
        memory_time_ns = rw_size / LOCAL_MEM_BW

        # translate into us and return
        memory_time_us = int(memory_time_ns / 1_000)
        return memory_time_us


class XmlSendStep(XmlStep):
    """Encapsulate an XML send step."""

    def __init__(
        self,
        tb_xml_node: ElementTree.Element,
        src: int,
        step_id: int,
        node_id: int,
        comm_data_size_bytes: int,
        src_offset: int,
        dst_offset: int,
    ) -> None:
        """
        Initialize XML send step.

        Args:
            tb_xml_node (ElementTree.Element): threadblock information <tb>
            src (int): src GPU id (<gpu id>)
            step_id (int): id of the step (<step s>)
            node_id (int): id of the chakra node
            comm_data_size_bytes (int): size of the communication (in bytes)
            src_offset (int): offset in src gpu buffer
            dst_offset (int): offset in dst gpu buffer
        """
        # superclass initializer
        super().__init__(node_id=node_id)

        # get additional attributes
        tb_id = int(tb_xml_node.attrib["id"])
        dest = int(tb_xml_node.attrib["send"])
        tag = int(tb_xml_node.attrib["chan"])

        # assign attributes to the Chakra node
        self._node.name = f"COMM_SEND_NODE_tb{tb_id}_step{step_id}"
        self._node.type = COMM_SEND_NODE
        self._node.attr.append(ChakraAttr(name="comm_type", int64_val=COMM_SEND_NODE))
        self._node.attr.append(ChakraAttr(name="comm_size", uint64_val=comm_data_size_bytes))
        self._node.attr.append(ChakraAttr(name="comm_src", uint32_val=src))
        self._node.attr.append(ChakraAttr(name="comm_dst", uint32_val=dest))
        self._node.attr.append(ChakraAttr(name="comm_tag", uint32_val=tag))
        self._node.attr.append(ChakraAttr(name="is_cpu_op", bool_val=False))
        self._node.attr.append(ChakraAttr(name="comm_src_off", uint32_val=src_offset))
        self._node.attr.append(ChakraAttr(name="comm_dst_off", uint32_val=dst_offset))


class XmlReceiveStep(XmlStep):
    """Encapsulate an XML recv step."""

    def __init__(
        self, tb_xml_node: ElementTree.Element, dest: int, step_id: int, node_id: int, comm_data_size_bytes: int
    ) -> None:
        """
        Initialize XML recv step.

        Args:
            tb_xml_node (ElementTree.Element): threadblock information <tb>
            dest (int): dest GPU id (<gpu id>)
            step_id (int): id of the step (<step s>)
            node_id (int): id of the chakra node
            comm_data_size_bytes (int): size of the communication (in bytes)
        """
        # superclass initializer
        super().__init__(node_id=node_id)

        # get additional attributes
        tb_id = tb_xml_node.attrib["id"]
        src = int(tb_xml_node.attrib["recv"])
        tag = int(tb_xml_node.attrib["chan"])

        # assign attributes to the Chakra node
        self._node.name = f"COMM_RECV_NODE_tb{tb_id}_step{step_id}"
        self._node.type = COMM_RECV_NODE
        self._node.attr.append(ChakraAttr(name="comm_type", int64_val=COMM_RECV_NODE))
        self._node.attr.append(ChakraAttr(name="comm_size", uint64_val=comm_data_size_bytes))
        self._node.attr.append(ChakraAttr(name="comm_src", uint32_val=src))
        self._node.attr.append(ChakraAttr(name="comm_dst", uint32_val=dest))
        self._node.attr.append(ChakraAttr(name="comm_tag", uint32_val=tag))
        self._node.attr.append(ChakraAttr(name="is_cpu_op", bool_val=False))


class XmlReceiveReduceComputeStep(XmlStep):
    """
    Encapsulate an XML recv-reduce-compute (rrc) step.

    Since this is a fused operation, this step carries multiple Chakra xml_steps.
    Therefore, the encode_message() function has been overriden accordingly.
    """

    def __init__(
        self,
        tb_xml_node: ElementTree.Element,
        dest: int,
        step_id: int,
        recv_node_id: int,
        comp_node_id: int,
        comm_data_size_bytes: int,
    ) -> None:
        """
        Initialize XML rrc step.

        Args:
            tb_xml_node (ElementTree.Element): threadblock information <tb>
            dest (int): dest GPU id (<gpu id>)
            step_id (int): id of the step (<step s>)
            recv_node_id (int): id of the Chakra recv node
            comp_node_id (int): id of the Chakra compute node
            comm_data_size_bytes (int): size of the communication (in bytes)
        """
        # self._node is the recv_node
        super().__init__(node_id=recv_node_id)

        # create another node for computation
        self._comp_node = Node()
        self._comp_node.id = comp_node_id

        # get additional attributes
        tb_id = tb_xml_node.attrib["id"]
        src = int(tb_xml_node.attrib["recv"])
        tag = int(tb_xml_node.attrib["chan"])

        # reduction step
        # assign recv node info
        self._node.name = f"COMM_RECV_NODE_tb{tb_id}_step{step_id}"
        self._node.type = COMM_RECV_NODE
        self._node.attr.append(ChakraAttr(name="comm_type", int64_val=COMM_RECV_NODE))
        self._node.attr.append(ChakraAttr(name="comm_size", uint64_val=comm_data_size_bytes))
        self._node.attr.append(ChakraAttr(name="comm_src", uint32_val=src))
        self._node.attr.append(ChakraAttr(name="comm_dst", uint32_val=dest))
        self._node.attr.append(ChakraAttr(name="comm_tag", uint32_val=tag))
        self._node.attr.append(ChakraAttr(name="is_cpu_op", bool_val=False))

        # compute step
        # assign comp node info
        self._comp_node.name = f"COMP_NODE_tb{tb_id}_step{step_id}"
        self._comp_node.type = COMP_NODE
        self._comp_node.duration_micros = XmlStep._calculate_reduction_time(data_size=comm_data_size_bytes)
        self._comp_node.attr.append(ChakraAttr(name="is_cpu_op", bool_val=False))

        # assign reduce-compute dependency
        self._comp_node.data_deps.append(self._node.id)

    def encode_message(self, g: BufferedWriter) -> None:
        """
        Encode both self._node and compute node to the protobuf file.

        Args:
            g (BufferedWriter): protobuf file to encode
        """
        encodeMessage(g, self._node)
        encodeMessage(g, self._comp_node)

    def _get_last_node(self) -> Node:
        """
        Return the last Chakra node of this XML step, i.e., comp node.

        Returns
            Node: Chakra comp node
        """
        return self._comp_node


class XmlNopStep(XmlStep):
    """
    Encapsulate an XML nop step.

    No Chakra node is associated with this since Nop isn't associated with any physical operation.
    """

    def __init__(self) -> None:
        """Initialize the base class."""
        super().__init__(-1)


class XmlConverter:
    """Converts an XML file to the corresponding Chakra-protobuf files."""

    def __init__(self, input_filename: str, output_filename: str, chunk_size: int) -> None:
        """
        Initialize XmlConverter class.

        Args:
            input_filename (str): path to the input xml custom collective file.
            output_filename (str): base path to write the converted .et file.
            chunk_size (int): size of individual chunk size (in bytes)
        """
        # check argument validity
        assert chunk_size is not None, "--chunk_size not defined."

        # assign attributes
        self.__input_filename = input_filename
        self.__output_filename = output_filename
        self.__chunk_size = chunk_size

        # reset next_node_id
        self.__next_node_id = -1
        self.__reset_node_id()

    def __encode_global_metadata(self, g: BufferedWriter) -> None:
        """
        Create the global metadata info and embed it into the protobuf file.

        Args:
            g (BufferedWriter): Chakra protobuf file
        """
        # construct metadata
        attr = [ChakraAttr(name="schema", string_val="1.0.2-chakra.0.0.4")]
        metadata = GlobalMetadata(attr=attr)

        # encode metadata
        encodeMessage(g, metadata)

    def __get_et_node_id(self) -> int:
        """
        Get new Chakra node id.

        Returns
            int: new Chakra node id
        """
        new_node_id = self.__next_node_id
        self.__next_node_id += 1

        return new_node_id

    def __reset_node_id(self) -> None:
        """Reset the new Chakra node id back to the initial value."""
        self.__next_node_id = 0

    def __parse_xml_file(
        self, xml: ElementTree.Element
    ) -> Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]:
        """
        Parse the given xml file.

        This function creates a dictionary of [gpu_id][tb_id][step_id] = (XmlStep, StepInfo)

        Args:
            xml (ElementTree.Element): xml tree

        Returns:
            Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]:
                parsed dict of (XmlStep, StepInfo) tuple.
        """
        # parsed xml_steps
        xml_steps: Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]] = dict()

        # Iterate over each GPU
        root = xml.getroot()
        for gpu in root.findall("gpu"):
            # read GPU metadata
            gpu_id = int(gpu.attrib["id"])
            chunks_count = int(gpu.attrib["i_chunks"])
            if chunks_count == 0:
                chunks_count = int(gpu.attrib["o_chunks"])

            # create new xml_steps entry
            xml_steps[gpu_id] = dict()

            # parse threadblocks of this gpu
            self.__parse_threadblocks(gpu_id=gpu_id, gpu=gpu, xml_steps=xml_steps)

        # return parsed xml_steps
        return xml_steps

    def __parse_threadblocks(
        self,
        gpu_id: int,
        gpu: ElementTree.Element,
        xml_steps: Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]],
    ) -> None:
        """
        Parse all threadblocks within a GPU.

        Args:
            gpu_id (int): id of the GPU currently being parsed
            gpu (ElementTree.Element): actual GPU info in the xml tree
            xml_steps (Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]):
                dictionary to be updated with the parsed result.
        """
        # Reset Chakra node id
        self.__reset_node_id()

        # iterate all threadblock information for target gpu
        for tb in gpu.findall("tb"):
            # prase threadblock metadata
            tb_id = int(tb.attrib["id"])

            # create new xml_steps entry
            xml_steps[gpu_id][tb_id] = dict()

            # parse steps of this threadblock
            self.__parse_steps(gpu_id=gpu_id, tb_id=tb_id, tb=tb, xml_steps=xml_steps)

    def __parse_steps(
        self,
        gpu_id: int,
        tb_id: int,
        tb: ElementTree.Element,
        xml_steps: Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]],
    ) -> None:
        """
        Parse all threadblocks within a GPU.

        Args:
            gpu_id (int): id of the GPU currently being parsed
            tb_id (int): id of the threadblock currently being parsed
            tb (ElementTree.Element): actual threadblock info in the xml tree
            xml_steps (Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]):
                dictionary to be updated with the parsed result.
        """
        # iterate all step information of the target threadblock
        for step in tb.findall("step"):
            # parse step metadata
            step_id = int(step.attrib["s"])
            chunks_count = int(step.attrib["cnt"])
            step_type = step.attrib["type"]
            src_offset = int(step.attrib["srcoff"])
            dst_offset = int(step.attrib["dstoff"])

            # data size
            data_size = self.__chunk_size * chunks_count

            # create Chakra node wrapper (XmlStep) based on step type
            et_node_id = self.__get_et_node_id()
            node = None
            if step_type == "s":
                node = XmlSendStep(
                    tb_xml_node=tb,
                    src=gpu_id,
                    step_id=step_id,
                    node_id=et_node_id,
                    comm_data_size_bytes=data_size,
                    src_offset=src_offset,
                    dst_offset=dst_offset,
                )
            elif step_type == "r":
                node = XmlReceiveStep(
                    tb_xml_node=tb, dest=gpu_id, step_id=step_id, node_id=et_node_id, comm_data_size_bytes=data_size
                )
            elif step_type == "rrc":
                comp_et_node_id = self.__get_et_node_id()
                node = XmlReceiveReduceComputeStep(
                    tb_xml_node=tb,
                    dest=gpu_id,
                    step_id=step_id,
                    recv_node_id=et_node_id,
                    comp_node_id=comp_et_node_id,
                    comm_data_size_bytes=data_size,
                )
            elif step_type == "nop":
                node = XmlNopStep()
            else:
                print(f"step type {step_type} not implemented in the converter")
                exit(-1)

            # insert parsed XmlStep to xml_steps
            xml_steps[gpu_id][tb_id][step_id] = node, step

    def __assign_parent_step_deps(
        self, xml_steps: Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]
    ) -> None:
        """
        Assign dependencies to the parent steps.

        Args:
            xml_steps (Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]):
                parsed xml_steps info dict.
        """
        # iterate over the xml_steps dictionary
        for gpu_id in xml_steps:
            for tb_id in xml_steps[gpu_id]:
                for step_id in xml_steps[gpu_id][tb_id]:
                    # if first step, skip
                    if step_id == 0:
                        continue

                    xml_step, _ = xml_steps[gpu_id][tb_id][step_id]

                    # if Nop, assign no dependency
                    if type(xml_step) is XmlNopStep:
                        continue

                    # get immediate prev step
                    prev_step_id = step_id - 1
                    prev_xml_step, prev_step_info = xml_steps[gpu_id][tb_id][prev_step_id]

                    # in case the immediate prev next step is Nop,
                    # track the tree until prev_xml_stpe is not Nop
                    # while assigning Nop dependencies
                    while type(prev_xml_step) is XmlNopStep:
                        # assign Nop dependency
                        dep_tb_id = int(prev_step_info.attrib["depid"])
                        dep_step_id = int(prev_step_info.attrib["deps"])
                        dep_node = xml_steps[gpu_id][dep_tb_id][dep_step_id]
                        xml_step.add_parent(dep_node)

                        # track previous step
                        prev_step_id = prev_step_id - 1
                        if prev_step_id < 0:
                            break
                        prev_xml_step = xml_steps[gpu_id][tb_id][prev_step_id]

                    # if Nop prev_step is found, assign the dependency
                    if type(prev_xml_step) is not XmlNopStep:
                        xml_step.add_parent(prev_xml_step)

    def __assign_inter_step_deps(
        self, xml_steps: Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]
    ) -> None:
        """
        Assign dependencies among XML steps.

        Args:
            xml_steps (Dict[int, Dict[int, Dict[int, Tuple[XmlStep, ElementTree.Element]]]]):
                parsed xml_steps info dict.
        """
        # iterate over the xml_steps dictionary
        for gpu_id in xml_steps:
            for tb_id in xml_steps[gpu_id]:
                for step_id in xml_steps[gpu_id][tb_id]:
                    xml_step, step_info = xml_steps[gpu_id][tb_id][step_id]

                    # if Nop, assign no dependency
                    if type(xml_step) is XmlNopStep:
                        continue

                    # get dependency information of this step
                    dep_tb_id = int(step_info.attrib["depid"])
                    dep_step_id = int(step_info.attrib["deps"])

                    # if no depencency, skip
                    if dep_tb_id == -1:
                        continue

                    # if inter-step dependency, assign it
                    parent_xml_step, _ = xml_steps[gpu_id][dep_tb_id][dep_step_id]
                    try:
                        xml_step.add_parent(parent_xml_step)
                    except Exception as e:
                        print(e, f"Parent assignment issue: {gpu_id}, {dep_tb_id}, {dep_tb_id}, {tb_id}, {step_id}")
                        exit(-1)

    def convert(self) -> None:
        """Convert the target xml file to Chakra-protobuf et files."""
        # parse the input xml file
        xml = ElementTree.parse(self.__input_filename)
        xml_steps = self.__parse_xml_file(xml=xml)

        # assign inter-step dependency information
        self.__assign_parent_step_deps(xml_steps=xml_steps)
        self.__assign_inter_step_deps(xml_steps=xml_steps)

        # store the result into protobuf .et files
        for gpu_id in xml_steps:
            # output filename
            output_filename = f"{self.__output_filename}.{gpu_id}.et"

            with open(output_filename, "wb") as g:
                # first, encode metadata
                self.__encode_global_metadata(g)

                # then, store all xml_step information
                for tb_id in xml_steps[gpu_id]:
                    for xml_step, _ in xml_steps[gpu_id][tb_id].values():
                        # skip Nop
                        if type(xml_step) is XmlNopStep:
                            continue

                        # encode message
                        xml_step.encode_message(g)
