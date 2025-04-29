if(CONFIG_COMPONENT_WEBNET_ENABLED)
    list(APPEND ADD_INCLUDE "inc"
                            )

    list(APPEND ADD_SRCS "src/webnet.c"
                         "src/wn_mimetype.c"
                         "src/wn_request.c"
                         "src/wn_session.c"
                         "src/wn_utils.c"
                         "src/wn_module.c"
                         )

    if(CONFIG_WEBNET_USING_ASP)
        list(APPEND ADD_SRCS "module/wn_module_asp.c"
                             )
    endif()

    if(CONFIG_WEBNET_USING_AUTH)
        list(APPEND ADD_SRCS "module/wn_module_auth.c"
                             )
    endif()
    
    if(CONFIG_WEBNET_USING_CGI)
        list(APPEND ADD_SRCS "module/wn_module_cgi.c"
                             )
    endif()

    if(CONFIG_WEBNET_USING_INDEX)
        list(APPEND ADD_SRCS "module/wn_module_index.c"
                             )
    endif()

    if(CONFIG_WEBNET_USING_ALIAS)
        list(APPEND ADD_SRCS "module/wn_module_alias.c"
                             )
    endif()
    
    if(CONFIG_WEBNET_USING_UPLOAD)
        list(APPEND ADD_SRCS "module/wn_module_upload.c"
                             )
    endif()
    
    if(CONFIG_WEBNET_USING_SSI)
        list(APPEND ADD_SRCS "module/wn_module_ssi.c"
                             )
    endif()

    if(CONFIG_WEBNET_USING_DAV)
        list(APPEND ADD_SRCS "module/wn_module_dav.c"
                             )
    endif()

    if(CONFIG_WEBNET_USING_LOG)
        list(APPEND ADD_SRCS "module/wn_module_log.c"
                             )
    endif()

    register_component()
    
endif()

