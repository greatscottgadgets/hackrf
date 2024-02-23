if(NOT DEFINED RELEASE)
	execute_process(
	        COMMAND git log -n 1 --format=%h
	        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	        RESULT_VARIABLE GIT_EXIT_VALUE
	        ERROR_QUIET
	        OUTPUT_VARIABLE GIT_VERSION
	        OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if (GIT_EXIT_VALUE)
	        set(RELEASE "2024.02.1+")
	else (GIT_EXIT_VALUE)
		execute_process(
			COMMAND git status -s --untracked-files=no
			OUTPUT_VARIABLE DIRTY
		)
		if ( NOT "${DIRTY}" STREQUAL "" )
			set(DIRTY_FLAG "*")
		else()
			set(DIRTY_FLAG "")
		endif()
	        set(RELEASE "git-${GIT_VERSION}${DIRTY_FLAG}")
	endif (GIT_EXIT_VALUE)
endif()
