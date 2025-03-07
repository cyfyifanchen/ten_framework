#
# Copyright © 2025 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
import logging

logger = logging.getLogger("default_extension_python")
logger.setLevel(logging.INFO)

formatter_str = (
    "%(asctime)s - %(name)s - %(levelname)s - %(process)d - "
    "[%(filename)s:%(lineno)d] - %(message)s"
)
formatter = logging.Formatter(formatter_str)

console_handler = logging.StreamHandler()
console_handler.setFormatter(formatter)

logger.addHandler(console_handler)
