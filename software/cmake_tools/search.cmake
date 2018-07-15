
# searches the given source folders for files matching the file patterns, then sets a variable in the parent scope
# (i.e the function that called this' scope) named ${list_name}
function(search "file_patterns" "folders" "list_name")
    # create a local list to store files into
    set("local-src")
    # use this varible to store the length of the searched folders
    set("length")
    # loop through each source folder
    foreach("folder" ${folders})
        # loop through each file pattern to find any matching files within the folder given
        foreach("pattern" ${file_patterns})
            file(GLOB_RECURSE "folder-src" "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/${pattern}")
            # get the length of the GLOBBED files and see if the list length is greater then 0
            list(LENGTH "folder-src" "length")
            if ("${length}" GREATER 0)
                # append the files to local-src list
                list(APPEND "local-src" "${folder-src}")
            endif()
        endforeach("pattern")
    endforeach("folder")
    # set the src variable to be our local-src list in the parent scope so that we can access it from the function
    # that called this functions. This acts as a sort of pseudo-return
    set(${list_name} "${local-src}" PARENT_SCOPE)
endfunction(search)