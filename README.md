# Chakra

Chakra is an open and interoperable graph-based representation of AI/ML workloads focused on enabling and accelerating AI SW/HW co-design. Chakra execution traces represent key operations, such as compute, memory, and communication, data and control dependencies, timing, and resource constraints.

This is a repository of Chakra schema and a complementary set of tools and capabilities to enable the collection, analysis, generation, and adoption of Chakra execution traces by a broad range of simulators, emulators, and replay tools.

Chakra is under active development as a [MLCommons](https://mlcommons.org/en) research project. Please see [MLCommons Chakra Working Group](https://mlcommons.org/en/groups/research-chakratracebench/) for more details for participating in this effort.

A detailed description of the original motivation and guiding principles can be found [here](https://arxiv.org/abs/2305.14516). The paper was published prior to Chakra becoming a MLCommons project. Please cite this repository to refer to the latest Chakra schema and tools.

## Installation
### Step 1: Set up a Virtual Environment
It's advisable to create a virtual environment using Python 3.10.2.

```bash
# Create a virtual environment
$ python3 -m venv chakra_env

# Activate the virtual environment
$ source chakra_env/bin/activate.csh
```
### Step 2: Install Chakra
With the virtual environment activated, install the Chakra package using pip.

```bash
# Install package from source
$ pip install .
$ source chakra_env/bin/activate.csh
```

### Step 3: Uninstalling Chakra
To uninstall Chakra, use the following command within the virtual environment.

```bash
$ pip uninstall chakra
```


## Execution Trace Converter (converter)
This tool converts execution traces into the Chakra format.
This converter supports three types of formats: ASTRA-sim text files, FlexFlow, and PyTorch.

You can use the following commands for each input type.

### ASTRA-sim Text Files
```shell
$ chakra_converter \
    --input_type Text\
    --input_filename <input_filename>\
    --output_filename <output_filename>\
    --num_npus <num_npus>\
    --num_passes <num_passes>
```

### FlexFlow Execution Traces
```shell
$ chakra_converter \
    --input_type FlexFlow\
    --input_filename <input_filename>\
    --output_filename <output_filename>\
    --npu_frequency <npu_frequency>
```

### PyTorch Execution Traces
```shell
$ chakra_converter \
    --input_type PyTorch\
    --input_filename <input_filename>\
    --output_filename <output_filename>
```

## Execution Trace Generator (generator)
This is an execution trace generator that generates synthetic execution traces.
A user can define a new function in the generator to generate new synthetic execution traces.
You can follow the commands below to run the generator.
```shell
$ chakra_generator\
    --num_npus <num_npus>\
    --model <model_name>
```


## Execution Trace Visualizer (visualizer)
This tool visualizes a given execution trace (ET) by converting the ET to a graphviz file.
A user has to feed the output graphviz file to a graphviz visualizer such as https://dreampuf.github.io/GraphvizOnline/.

You can run this tool with the following command.
```shell
$  chakra_visualizer\
    --input_filename <input_filename>\
    --output_filename <output_filename>
```
## Execution Trace Jsonizer (jsonizer)
This tool prints the nodes within execution traces for better comprehension.
The printed information includes the node's id, name, type, and any associated metadata, which are all outputted in a user-friendly text format.
```
$ chakra_jsonizer\
    --input_filename <input_filename>\
    --output_filename <output_filename>
```

## Timeline Visualizer (timeline_visualizer)
This tool visualizes the execution timeline of a given execution trace (ET).
```shell
$ chakra_timeline_visualizer\
    --input_filename <input_filename>\
    --output_filename <output_filename>\
    --num_npus <num_npus>\
    --npu_frequency <npu_frequency>
```

The input file is an execution trace file in csv, and the output file is a json file.
The input file format is shown below.
```csv
issue,<dummy_str>=npu_id,<dummy_str>=curr_cycle,<dummy_str>=node_id,<dummy_str>=node_name
callback,<dummy_str>=npu_id,<dummy_str>=curr_cycle,<dummy_str>=node_id,<dummy_str>=node_name
issue,<dummy_str>=npu_id,<dummy_str>=curr_cycle,<dummy_str>=node_id,<dummy_str>=node_name
issue,<dummy_str>=npu_id,<dummy_str>=curr_cycle,<dummy_str>=node_id,<dummy_str>=node_name
callback,<dummy_str>=npu_id,<dummy_str>=curr_cycle,<dummy_str>=node_id,<dummy_str>=node_name
callback,<dummy_str>=npu_id,<dummy_str>=curr_cycle,<dummy_str>=node_id,<dummy_str>=node_name
...
```
As this tool requires an execution trace of an ET, a simulator has to print out execution traces.
The output json file is chrome-tracing-compatible.
When you open the file with `chrome://tracing`, you will see an execution timeline like the one below.
![](doc/timeline_visualizer.png)


## Execution Trace Feeder (et_feeder)
This is a trace feeder that feeds dependency-free nodes to a simulator.
Therefore, a simulator has to import this feeder as a library.
Currently, ASTRA-sim is the only simulator that supports the trace feeder.
You can run execution traces on ASTRA-sim with the following commands.
```
$ git clone --recursive git@github.com:astra-sim/astra-sim.git
$ cd astra-sim
$ git checkout Chakra
$ git submodule update --init --recursive
$ cd extern/graph_frontend/chakra/
$ git checkout main
$ cd -
$ ./build/astra_analytical/build.sh -c

$ cd extern/graph_frontend/chakra/
$ python -m chakra.et_generator.et_generator\
    --num_npus <num_npus>

$ cd -
$ ./run.sh
```


Check out [`USER_GUIDE`](USER_GUIDE.md) for details.

## License

Chakra is released under the MIT license. Please see the [`LICENSE.md`](LICENSE.md) file for more information.

## Contributing

We actively welcome your pull requests! Please see [`CONTRIBUTING.md`](CONTRIBUTING.md) for more info.
