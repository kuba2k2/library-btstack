/* Copyright (c) Kuba Szczodrzyński 2023-05-09. */

#pragma once

#include <btstack_port_config.h>

#ifdef __has_include
#if __has_include(<btstack_user_config.h>)
#include <btstack_user_config.h>
#else
#include <btstack_default_config.h>
#endif
#else
#include <btstack_default_config.h>
#endif
