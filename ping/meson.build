# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) Iputils Project, 2017-2021

inc = include_directories('..')

cfiles = [
	'ping.c',
	'ping_common.c',
	'ping6_common.c',
	'node_info.c',
	git_version_h
]

dependencies = [
	cap_dep,
	idn_dep,
	intl_dep,
	m_dep,
	resolv_dep
]

if (json)
  cfiles += 'ping_json.c'
  dependencies += json_dep
else
  cfiles += 'ping_json_dummy.c'
endif

ping = executable('ping', cfiles,
	include_directories : inc,
	dependencies : dependencies,
	link_with : [libcommon],
	install: true)

if (setcap_ping)
	meson.add_install_script('../build-aux/setcap-setuid.sh',
		bindir,
		'ping',
		perm_type,
		setcap_path
	)
endif
