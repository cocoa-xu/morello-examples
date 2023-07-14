# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

define create-archive
	@rm -f $@
	$(AR) rc $@ $^
	$(RANLIB) $@
endef
