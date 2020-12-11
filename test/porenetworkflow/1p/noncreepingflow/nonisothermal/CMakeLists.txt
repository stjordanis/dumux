add_input_file_links()

dune_add_test(NAME test_pnm_1pni_noncreeping_flow
              SOURCES main.cc
              LABELS porenetworkflow
              COMPILE_DEFINITIONS ISOTHERMAL=0
              COMMAND ${CMAKE_SOURCE_DIR}/bin/testing/runtest.py
              CMD_ARGS       --script fuzzy
                             --files ${CMAKE_SOURCE_DIR}/test/references/test_pnm_1pni_noncreepingflow-reference.vtp
                                     ${CMAKE_CURRENT_BINARY_DIR}/test_pnm_1pni_noncreepingflow-00017.vtp
                             --command "${CMAKE_CURRENT_BINARY_DIR}/test_pnm_1pni_noncreeping_flow")
