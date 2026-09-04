"""
Copyright 2020 Google LLC
Copyright 2020 PerfectVIPs Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

"""
import sys
import logging
import pandas as pd
from tabulate import tabulate
from pygen_src.riscv_instr_gen_config import cfg
from pygen_src.riscv_loop_instr import riscv_loop_instr
from pygen_src.riscv_directed_instr_lib import (riscv_directed_instr_stream,
                                                riscv_int_numeric_corner_stream,
                                                riscv_jal_instr, riscv_mem_access_stream)
from pygen_src.riscv_amo_instr_lib import (riscv_lr_sc_instr_stream, riscv_amo_instr_stream)
from pygen_src.riscv_load_store_instr_lib import (riscv_load_store_rand_instr_stream,
                                                  riscv_load_store_hazard_instr_stream,
                                                  riscv_load_store_stress_instr_stream,
                                                  riscv_single_load_store_instr_stream)


# ----------------------------------------------------------
# pyflow commmon utility helpers functions
# ----------------------------------------------------------

_DIRECTED_INSTR_STREAM_FACTORY = {
    "riscv_directed_instr_stream": riscv_directed_instr_stream,
    "riscv_int_numeric_corner_stream": riscv_int_numeric_corner_stream,
    "riscv_jal_instr": riscv_jal_instr,
    "riscv_mem_access_stream": riscv_mem_access_stream,
    "riscv_lr_sc_instr_stream": riscv_lr_sc_instr_stream,
    "riscv_amo_instr_stream": riscv_amo_instr_stream,
    "riscv_load_store_rand_instr_stream": riscv_load_store_rand_instr_stream,
    "riscv_load_store_hazard_instr_stream": riscv_load_store_hazard_instr_stream,
    "riscv_load_store_stress_instr_stream": riscv_load_store_stress_instr_stream,
    "riscv_single_load_store_instr_stream": riscv_single_load_store_instr_stream,
    "riscv_loop_instr": riscv_loop_instr
}

# ``_DIRECTED_INSTR_STREAM_FACTORY`` is also used by the legacy, explicitly
# configured directed-stream path.  Keep all of those entries available there
# because some streams are implemented only for the SV flow (or are useful to
# existing users even though they cannot be embedded by PyFlow).  Random
# injection has a stricter contract: constructing and randomizing a stream must
# produce a usable, non-empty instruction list.  The base stream and the
# memory/AMO base streams do not meet that contract today (the former has no
# generator, while the latter two hit known PyVSC list randomization bugs), so
# keep a separate allow-list for this path.  Each backend exposes all streams
# that it can safely construct as an embedded random sequence; the SV backend
# has additional implementations and applies its own target/config filtering.
_PYFLOW_RANDOM_DIRECTED_INSTR_STREAM_NAMES = frozenset({
    "riscv_int_numeric_corner_stream",
    "riscv_jal_instr",
    "riscv_load_store_rand_instr_stream",
    "riscv_load_store_hazard_instr_stream",
    "riscv_load_store_stress_instr_stream",
    "riscv_single_load_store_instr_stream",
    "riscv_loop_instr",
})


def is_directed_instr_stream_supported(name):
    """Return whether the Python backend can construct a directed stream."""
    return name in _DIRECTED_INSTR_STREAM_FACTORY


def is_random_directed_instr_stream_supported(name):
    """Return whether *name* is safe for PyFlow random stream injection.

    This deliberately differs from :func:`is_directed_instr_stream_supported`:
    the latter preserves the historical factory/API surface, whereas random
    injection must reject classes that construct successfully but cannot be
    randomized into a valid embedded stream.
    """
    if name not in _PYFLOW_RANDOM_DIRECTED_INSTR_STREAM_NAMES:
        return False
    if name in {"riscv_loop_instr", "riscv_jal_instr"}:
        try:
            if int(cfg.no_branch_jump):
                return False
        except (TypeError, ValueError, AttributeError):
            pass
    # Load/store streams emit ``la`` references to generated data sections.
    # When data-page generation is disabled those symbols do not exist, even
    # though the stream itself can be randomized successfully.  Treat this as
    # an ineligible random candidate so a no-data-page test remains linkable.
    if (name.startswith("riscv_load_store_") or
            name == "riscv_single_load_store_instr_stream"):
        try:
            if int(cfg.no_data_page) or int(cfg.no_load_store):
                return False
        except (TypeError, ValueError, AttributeError):
            # Keep the conservative allow-list behavior if the configuration
            # object is not fully initialized (e.g. during module import).
            pass
    return True


def get_random_directed_instr_stream_names():
    """Return every random-injection stream supported by this PyFlow config."""
    return sorted(name for name in _PYFLOW_RANDOM_DIRECTED_INSTR_STREAM_NAMES
                  if is_random_directed_instr_stream_supported(name))


def factory(obj_of):
    objs = _DIRECTED_INSTR_STREAM_FACTORY

    try:
        return objs[obj_of]()
    except KeyError:
        logging.critical("Cannot Create object of %s", obj_of)
        sys.exit(1)


def gen_config_table():
    data = []
    for key, value in cfg.__dict__.items():
        # Ignoring the unneccesary attributes
        if key in ["_ro_int", "_int_field_info", "argv", "mem_region",
                   "amo_region", "s_mem_region", "args_dict"]:
            continue
        else:
            try:  # Fields values for the pyvsc data types
                data.append([key, type(key), sys.getsizeof(key), value.get_val()])
            except Exception:
                data.append([key, type(key), sys.getsizeof(key), value])
    df = pd.DataFrame(data, columns=['Name', 'Type', 'Size', 'Value'])
    df['Value'] = df['Value'].apply(str)
    logging.info('\n' + tabulate(df, headers='keys', tablefmt='psql'))
