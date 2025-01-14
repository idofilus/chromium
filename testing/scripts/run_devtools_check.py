#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs a python script under an isolate

This script attempts to emulate the contract of gtest-style tests
invoked via recipes. The main contract is that the caller passes the
argument:

  --isolated-script-test-output=[FILENAME]

json is written to that file in the format produced by
common.parse_common_test_results.

This script is intended to be the base command invoked by the isolate,
followed by a subsequent Python script."""

import argparse
import json
import os
import sys


import common

# Add src/testing/ into sys.path for importing xvfb.
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import xvfb


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--isolated-script-test-output', type=str,
                      required=True)
  args, rest_args = parser.parse_known_args()
  # Remove the chartjson extra arg until this script cares about chartjson
  # results
  index = 0
  for arg in rest_args:
    if ('--isolated-script-test-chartjson-output' in arg or
        '--isolated-script-test-perf-output' in arg or
        '--isolated-script-test-filter-file' in arg):
      rest_args.pop(index)
      break
    index += 1

  ret = common.run_command([sys.executable] + rest_args)
  with open(args.isolated_script_test_output, 'w') as fp:
    json.dump({'valid': True,
               'failures': ['failed'] if ret else []}, fp)
  return ret


# This is not really a "script test" so does not need to manually add
# any additional compile targets.
def main_compile_targets(args):
  json.dump([''], args.output)


if __name__ == '__main__':
  # Conform minimally to the protocol defined by ScriptTest.
  if 'compile_targets' in sys.argv:
    funcs = {
      'run': None,
      'compile_targets': main_compile_targets,
    }
    sys.exit(common.run_script(sys.argv[1:], funcs))
  sys.exit(main())
