### 🔍 Project Overview

**GeneralSparse** is an open-source project aimed at optimizing Sparse Matrix Multiplication (SpMM) on GPUs, particularly for diverse sparsity patterns in scientific computing and inference on GPUs.

Its core goals include:

- **Bridging the performance gap** of SpMM in pruned LLM inference on GPUs.
- **Supporting various sparsity patterns**, including both structured (e.g., N:M) and unstructured pruning.
- **Leveraging GPU parallelism** through custom CUDA implementations to accelerate sparse computations.

#### Environment
- CUDA 12.1 and NVIDIA A100/V100.

---

### 🗂️ Project Structure

The main components of the repository:

- **`cuda_code/`**: Contains CUDA head file for high-performance sparse matrix operations on the GPU.
- **`operator/`**: Defines sparse operation interfaces and logic, structured for maintainability and extension.
- **`kernel_token/`** & **`reduction_token/`**: Handle kernel identification and reduction operations to streamline computation.
- **`transform_step/`**: Data preparation modules that convert inputs into formats suitable for sparse multiplication.
- **`configor/`**: A configuration system that allows tuning for different models and hardware setups.
- **`baseline/`**: Includes baseline implementations for performance comparisons.
- **`code_generator.*` & `code_builder.*`**: Auto-generate and compile code tailored to specific sparsity patterns.
- **`data_transform_*.*`**: Utilities for transforming data into sparse-compatible formats.
- **`matrix_example/`**: Prepares the input matrix of pruned weight matrix and suitesparse matrix collection.
- **`data_source/`**: Contains the output generated SpMM program when running our method.

---

### ⚙️ Technical Highlights

- **Custom CUDA Kernels**: Designed specifically for sparse matrix multiplications to maximize GPU efficiency.
- **Modular Design**: Each module is decoupled for flexibility and scalability.
- **Auto Code Generation**: The `code_generator` and `code_builder` automate the generation of optimized kernels for various sparsity formats.
- **Cost Model**: Currently, the cost model is still under development and will be further improved in future work.


---

### 📈 Use Cases

- **Inference of pruned LLMs**: Speeds up inference while maintaining model accuracy.
- **High-performance computing**: Suitable for scientific computing or engineering scenarios involving large-scale sparse matrix data.

---

### Getting Started Instructions

#### ❗️ Use Steps for Input Sparse Matrix

To fully utilize **GeneralSparse**, follow the step:

1. **Prepare your sparse matrix**: Choose the appropriate sparse file .mtx format as in **`matrix_example/`**.
2. **Configurable Parameters**: configure the parameter by `global_config.json` file. In this file, modify the `ROOT_PATH_STR` and `spmv_header_file` by the directory location, and adjust `HALF` whether to use half precision.
3. **Compile the project**: Run `make token_test -j16` to generate executable file `./token_test`.
4. **Generate the tailed program for sparse matrix**: Run `./token_test matrix_example/suite_collection/IG5-18.mtx 8` and `8` is the column number of dense matrix and can be adjusted.
5. **View the generated code program**: The generated programs can be viewed in **`data_source/`** directory and can be executed by `a.out` in sub-directory.
6. **Other baselines**: The other methods can be viewed in **`baseline/`** directory. Here, we provide the code implementation of cuSPARSE, and other methods are provided by their Github repo-link.

#### ❕ Use Steps for Large Language Models

1. **Integration with models**: Our method uses the [FasterTransformer](https://github.com/NVIDIA/FasterTransformer) framework to accelerate the model end-to-end.
2. **Pruning the model**: The pruned weight matrix is instantiated to the **`matrix_example/pruned_weight`** location.
3. **Replace origin Library calls**: The end-to-end usage is similar to [Flash-LLM](https://github.com/AlibabaResearch/flash-llm). 
4. **Here**, we do not directly provide model-level code and binary implementations.

---

### 🧠 Other README.md position

1. **`baseline/`**: Each baseline has the guidence to generate executable file and command.
2. **`matrix_example/`**: Guidence to generate the sparse matrix input.

---


### 📚 Summary

**GeneralSparse** is a powerful and flexible framework for efficient sparse matrix multiplication on GPUs, ideal for pruned LLM inference. Its modular architecture and automation tools make it easy to integrate, extend, and adapt to various scenarios.

A highly recommended tool for developers aiming to deploy performant, sparse-aware models in real-world systems.
