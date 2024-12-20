#
# Copyright © 2024 Agora
# This file is part of TEN Framework, an open source project.
# Licensed under the Apache License, Version 2.0, with certain conditions.
# Refer to the "LICENSE" file in the root directory for more information.
#
from libten_runtime_python import _Data


class Data(_Data):
    def __init__(self):
        raise NotImplementedError("Use Data.create instead.")

    @classmethod
    def create(cls, msg_name: str):
        instance = cls.__new__(cls)
        instance.set_name(msg_name)
        return instance

    def alloc_buf(self, size: int) -> None:
        return _Data.alloc_buf(self, size)

    def lock_buf(self) -> memoryview:
        return _Data.lock_buf(self)

    def unlock_buf(self, buf: memoryview) -> None:
        return _Data.unlock_buf(self, buf)

    def get_buf(self) -> bytearray:
        return _Data.get_buf(self)
