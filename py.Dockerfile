ARG PYTHON_VERSION=latest
FROM python:${PYTHON_VERSION} as base

WORKDIR /build

# cache a layer of python + deps
COPY setup.py .
COPY README.md .
RUN python -m pip install --upgrade pip
RUN pip install flake8 pytest pybind11 numpy

FROM base AS build
COPY include/ include/
COPY src/ src/
COPY tests/ tests/
RUN pip install -e .
RUN pytest

ENTRYPOINT ["python"]
