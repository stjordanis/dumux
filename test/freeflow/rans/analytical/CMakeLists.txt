add_input_file_links()

dumux_add_test(NAME test_ff_rans_analytical_komega
              SOURCES main.cc
              COMPILE_DEFINITIONS TYPETAG=AnalyticalKOmega
              LABELS freeflow rans
              CMAKE_GUARD HAVE_UMFPACK
              COMMAND test_ff_rans_analytical_komega
              CMD_ARGS params.input)
