foreach(required_var SOURCE_DIR BINARY_DIR)
    if(NOT DEFINED ${required_var})
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef stage_id)
set(stage_dir "/tmp/nickelupdater-koboroot-${stage_id}")
set(package_files
    "libnickelupdater.so|usr/local/Kobo/imageformats/libnickelupdater.so"
    "config/nickelupdater.conf.tmpl|usr/local/nickelupdater/nickelupdater.conf.tmpl"
)
set(package_paths)
foreach(file_pair IN LISTS package_files)
    string(REPLACE "|" ";" file_pair "${file_pair}")
    list(GET file_pair 0 source)
    list(GET file_pair 1 destination)
    get_filename_component(destination_dir "${stage_dir}/${destination}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_dir}")
    if(source MATCHES "^config/")
        set(source_path "${SOURCE_DIR}/${source}")
    else()
        set(source_path "${BINARY_DIR}/${source}")
    endif()
    file(COPY_FILE "${source_path}" "${stage_dir}/${destination}")
    list(APPEND package_paths "${destination}")
endforeach()

file(CHMOD "${stage_dir}/usr/local/Kobo/imageformats/libnickelupdater.so"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

execute_process(
    COMMAND sh -c
        "stage_dir=$1; archive=$2; shift 2; trap 'rm -rf -- \"$stage_dir\"' EXIT; cd \"$stage_dir\"; tar --owner=root --group=root --mode=u=rwX,go=rX -czf \"$archive\" \"$@\""
        sh "${stage_dir}" "${SOURCE_DIR}/KoboRoot.tgz" ${package_paths}
    RESULT_VARIABLE tar_result
)
if(NOT tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create KoboRoot.tgz (tar exit status ${tar_result})")
endif()
message(STATUS "Created ${SOURCE_DIR}/KoboRoot.tgz")
