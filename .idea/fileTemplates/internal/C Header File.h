#parse("C File Header.h")
#set($INCLUDE_GUARD = "${PROJECT_NAME.toUpperCase()}_${FILE_NAME.toUpperCase().replace('-','_').replace('.','_')}")
#[[#ifndef]]# ${INCLUDE_GUARD}
#[[#define]]# ${INCLUDE_GUARD}

#[[#endif]]# // ${INCLUDE_GUARD}
