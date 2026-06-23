# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

add_executable(dcc-insider-dm-switch
    dcc-insider-dm-switch.cpp
)

set_target_properties(dcc-insider-dm-switch PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
)

install(TARGETS dcc-insider-dm-switch
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/dcc-insider-plugin
)

set(POLICY_NAME org.deepin.dcc.insider.dm-switch)
set(POLICY_ACTION_DIR ${CMAKE_SOURCE_DIR}/misc/polkit-action)
set(POLICY_TS_DIR ${CMAKE_SOURCE_DIR}/misc/ts/${POLICY_NAME}.policy)
set(POLICY_IN_FILE ${POLICY_ACTION_DIR}/${POLICY_NAME}.policy.in)
set(POLICY_SOURCE_FILE ${POLICY_ACTION_DIR}/${POLICY_NAME}.policy)
set(POLICY_GENERATED_FILE ${CMAKE_CURRENT_BINARY_DIR}/${POLICY_NAME}.policy)
file(GLOB POLICY_TS_FILES CONFIGURE_DEPENDS ${POLICY_TS_DIR}/policy*.ts)

add_custom_target(policy_ts
    COMMAND deepin-policy-ts-convert policy2ts ${POLICY_IN_FILE} ${POLICY_TS_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Update policy TS files from ${POLICY_NAME}.policy.in"
)

add_custom_target(policy_ts_to_policy
    COMMAND deepin-policy-ts-convert ts2policy ${POLICY_IN_FILE} ${POLICY_TS_DIR} ${POLICY_SOURCE_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Update ${POLICY_NAME}.policy from policy TS files"
)

add_custom_command(
    OUTPUT ${POLICY_GENERATED_FILE}
    COMMAND deepin-policy-ts-convert ts2policy ${POLICY_IN_FILE} ${POLICY_TS_DIR} ${POLICY_GENERATED_FILE}
    DEPENDS ${POLICY_IN_FILE} ${POLICY_TS_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generate ${POLICY_NAME}.policy for installation"
    VERBATIM
)

add_custom_target(policy ALL
    DEPENDS ${POLICY_GENERATED_FILE}
)

install(FILES ${POLICY_GENERATED_FILE}
    DESTINATION share/polkit-1/actions
)
