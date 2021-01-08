#Running Code analysis locally

The TianoCore EDKII project has introduced Core CI infrastructure using TianoCore EDKII Tools PIP modules:

   -  *[edk2-pytool-library](https://pypi.org/project/edk2-pytool-library)*

   - *[edk2-pytool-extensions](https://pypi.org/project/edk2-pytool-extensions)*

The instructions to setup the CI environment are in *'edk2\\.pytool\\Readme.md'*

VExpressPkg is part of Platform CI for builds so the steps below are only used for running
code analysis locally.

## Running code analysis on VExpressPkg with Pytools

1. [Optional] Create a Python Virtual Environment - generally once per workspace

    ```
        python -m venv <name of virtual environment>

        e.g. python -m venv edk2-ci
    ```

2. [Optional] Activate Virtual Environment - each time new shell/command window is opened

    ```
        <name of virtual environment>/Scripts/activate

        e.g. On a windows host PC run:
             edk2-ci\Scripts\activate.bat
    ```
3. Install Pytools - generally once per virtual env or whenever pip-requirements.txt changes

    ```
        pip install --upgrade -r pip-requirements.txt
    ```

4. Initialize & Update Submodules - only when submodules updated

    ```
        stuart_setup -c .pytool/CISettings.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

        e.g. stuart_setup -c .pytool/CISettings.py TOOL_CHAIN_TAG=GCC5
    ```

5. Initialize & Update Dependencies - only as needed when ext_deps change

    ```
        stuart_update -c .pytool/CISettings.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

        e.g. stuart_update -c .pytool/CISettings.py TOOL_CHAIN_TAG=GCC5
    ```

6. Compile the Basetools if necessary - only when Basetools C source files change

    ```
        python edk2/BaseTools/Edk2ToolsBuild.py -t <ToolChainTag>
    ```

7. Run code analysis on VExpressPkg

    ```
        stuart_build-c .pytool/CISettings.py TOOL_CHAIN_TAG=<TOOL_CHAIN_TAG> -a <TARGET_ARCH>

        e.g. stuart_ci_build -c .pytool/CISettings.py TOOL_CHAIN_TAG=GCC5 -p VExpressPkg -a AARCH64 --verbose
    ```

    - use `stuart_build -c .pytool/CISettings.py -h` option to see help on additional options.

