DBGROUP_ADD_TEST("operation_test")

DBGROUP_ADD_TEST("operation_engine_test")

DBGROUP_ADD_TEST("pmwcas_target_test")
target_sources(pmwcas_target_test PRIVATE
  "${PROJECT_SOURCE_DIR}/src/array/pmwcas_target.cpp"
)
