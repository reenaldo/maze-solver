# Problem Solving with Algorithms

Small C project implementing graph/maze algorithms (BFS, maze generation/solving, and helpers). The repository includes a `Makefile`, tests in `tests/`, and helper headers in `inc/`.

## Prerequisites

- Python 3 must be installed on your system. The visualization helper `scripts/visualize_maze.py` requires Python; without Python you won't be able to view the maze visualization even if the C program builds and runs. If Python isn't installed, install it (for example via your OS package manager or from https://python.org) before running `make run` or creating the virtual environment manually.

- Linux users: ensure the system Python 3 and the venv support are installed. On Debian/Ubuntu this is typically:
  ```zsh
  sudo apt update
  sudo apt install python3 python3-venv python3-pip
  ```
  On other distributions, install the equivalent packages (e.g. using `dnf`, `pacman`, etc.) so `python3 -m venv` and `pip` are available.

## Quick overview

- To build and run everything with an automated environment setup, use:

```zsh
make run
```

`make run` will:

- create the Python virtual environment (required for visualization / scripts)
- automatically install Python requirements from `requirements.txt`
- compile the C program(s) using the `Makefile`

This target is the easiest way to get the project working end-to-end.

If you prefer to handle the environment yourself, you can just run:

```zsh
make
```

Notes for the manual flow:

- `make` will compile the C sources, but it does not create the Python virtual environment or install `requirements.txt` automatically — you must create the env and install the requirements yourself if you need to run the Python scripts in `scripts/` (e.g. `visualize_maze.py`).

Example manual steps (if you choose not to use `make run`):

```zsh
# create a virtual environment (example)
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

# then build the C project
make
```

## Tests

Tests are located in the `tests/` folder. The `Makefile` exposes convenient targets:

- Run both test suites (BFS and maze) at once:

```zsh
make test
```

- Run a single test suite:

```zsh
make bfs_tests    # run BFS tests only
make maze_tests   # run Maze tests only
```

Each test target compiles and runs the corresponding test executable.

## How to run the compiled program

After `make` (or `make run`) the produced executable(s) will be available according to the `Makefile` rules (commonly `bin/` or the repository root depending on the Makefile). See the `Makefile` for exact paths and runtime flags.

## Contributing / Notes

- If you add Python tooling or scripts, prefer adding any new dependencies to `requirements.txt` so `make run` stays useful.
- If you modify test behavior, update the `Makefile` test targets accordingly.

---

## Members

- ADAM Cassandra
- AIT AIDER Sarah
- BAH Elhadj Abdoulaye
- DIALLO Mamadou Sanou
- MAACHE Mohamed
- SCHMITZ Laurent
- TRASHI Renaldo
