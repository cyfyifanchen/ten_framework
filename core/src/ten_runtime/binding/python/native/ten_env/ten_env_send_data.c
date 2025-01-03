//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/binding/python/common/common.h"
#include "include_internal/ten_runtime/binding/python/common/error.h"
#include "include_internal/ten_runtime/binding/python/msg/data.h"
#include "include_internal/ten_runtime/binding/python/msg/msg.h"
#include "include_internal/ten_runtime/binding/python/ten_env/ten_env.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/macro/memory.h"

typedef struct ten_env_notify_send_data_info_t {
  ten_shared_ptr_t *c_data;
  PyObject *py_cb_func;
} ten_env_notify_send_data_info_t;

static ten_env_notify_send_data_info_t *ten_env_notify_send_data_info_create(
    ten_shared_ptr_t *c_data, PyObject *py_cb_func) {
  TEN_ASSERT(c_data, "Invalid argument.");

  ten_env_notify_send_data_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_send_data_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->c_data = c_data;
  info->py_cb_func = py_cb_func;

  if (py_cb_func != NULL) {
    Py_INCREF(py_cb_func);
  }

  return info;
}

static void ten_env_notify_send_data_info_destroy(
    ten_env_notify_send_data_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  if (info->c_data) {
    ten_shared_ptr_destroy(info->c_data);
    info->c_data = NULL;
  }

  info->py_cb_func = NULL;

  TEN_FREE(info);
}

static void proxy_send_data_callback(ten_env_t *ten_env,
                                     TEN_UNUSED ten_shared_ptr_t *cmd_result,
                                     void *callback_info, ten_error_t *err) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(callback_info, "Should not happen.");

  // About to call the Python function, so it's necessary to ensure that the GIL
  // has been acquired.
  //
  // Allows C codes to work safely with Python objects.
  PyGILState_STATE prev_state = ten_py_gil_state_ensure();

  ten_py_ten_env_t *py_ten_env = ten_py_ten_env_wrap(ten_env);
  PyObject *cb_func = callback_info;

  PyObject *arglist = NULL;
  ten_py_error_t *py_error = NULL;

  if (err) {
    py_error = ten_py_error_wrap(err);

    arglist = Py_BuildValue("(OO)", py_ten_env->actual_py_ten_env, py_error);
  } else {
    arglist = Py_BuildValue("(OO)", py_ten_env->actual_py_ten_env, Py_None);
  }

  PyObject *result = PyObject_CallObject(cb_func, arglist);
  Py_XDECREF(result);  // Ensure cleanup if an error occurred.

  bool err_occurred = ten_py_check_and_clear_py_error();
  TEN_ASSERT(!err_occurred, "Should not happen.");

  Py_XDECREF(arglist);
  Py_XDECREF(cb_func);

  if (py_error) {
    ten_py_error_invalidate(py_error);
  }

  ten_py_gil_state_release(prev_state);
}

static void ten_env_proxy_notify_send_data(ten_env_t *ten_env,
                                           void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  ten_env_notify_send_data_info_t *notify_info = user_data;
  TEN_ASSERT(notify_info, "Invalid argument.");

  ten_error_t err;
  ten_error_init(&err);

  bool res = false;
  if (notify_info->py_cb_func == NULL) {
    res = ten_env_send_data(ten_env, notify_info->c_data, NULL, NULL, &err);
  } else {
    res = ten_env_send_data(ten_env, notify_info->c_data,
                            proxy_send_data_callback, notify_info->py_cb_func,
                            &err);
    if (!res) {
      // About to call the Python function, so it's necessary to ensure that the
      // GIL
      // has been acquired.
      //
      // Allows C codes to work safely with Python objects.
      PyGILState_STATE prev_state = ten_py_gil_state_ensure();

      ten_py_ten_env_t *py_ten_env = ten_py_ten_env_wrap(ten_env);
      ten_py_error_t *py_err = ten_py_error_wrap(&err);

      PyObject *arglist =
          Py_BuildValue("(OO)", py_ten_env->actual_py_ten_env, py_err);

      PyObject *result = PyObject_CallObject(notify_info->py_cb_func, arglist);
      Py_XDECREF(result);  // Ensure cleanup if an error occurred.

      bool err_occurred = ten_py_check_and_clear_py_error();
      TEN_ASSERT(!err_occurred, "Should not happen.");

      Py_XDECREF(arglist);
      Py_XDECREF(notify_info->py_cb_func);

      ten_py_error_invalidate(py_err);

      ten_py_gil_state_release(prev_state);
    }
  }

  ten_error_deinit(&err);

  ten_env_notify_send_data_info_destroy(notify_info);
}

PyObject *ten_py_ten_env_send_data(PyObject *self, PyObject *args) {
  ten_py_ten_env_t *py_ten_env = (ten_py_ten_env_t *)self;
  TEN_ASSERT(py_ten_env && ten_py_ten_env_check_integrity(py_ten_env),
             "Invalid argument.");

  bool success = true;

  ten_error_t err;
  ten_error_init(&err);

  ten_py_data_t *py_data = NULL;
  PyObject *cb_func = NULL;

  if (!PyArg_ParseTuple(args, "O!O", ten_py_data_py_type(), &py_data,
                        &cb_func)) {
    success = false;
    ten_py_raise_py_type_error_exception(
        "Invalid argument type when send data.");
    goto done;
  }

  // Check if cb_func is callable.
  if (!PyCallable_Check(cb_func)) {
    cb_func = NULL;
  }

  ten_shared_ptr_t *cloned_data = ten_shared_ptr_clone(py_data->msg.c_msg);
  ten_env_notify_send_data_info_t *notify_info =
      ten_env_notify_send_data_info_create(cloned_data, cb_func);

  if (!ten_env_proxy_notify(py_ten_env->c_ten_env_proxy,
                            ten_env_proxy_notify_send_data, notify_info, false,
                            &err)) {
    if (cb_func) {
      Py_XDECREF(cb_func);
    }

    ten_env_notify_send_data_info_destroy(notify_info);
    success = false;
    ten_py_raise_py_runtime_error_exception("Failed to send data.");
    goto done;
  } else {
    // Destroy the C message from the Python message as the ownership has been
    // transferred to the notify_info.
    ten_py_msg_destroy_c_msg(&py_data->msg);
  }

done:
  ten_error_deinit(&err);

  if (success) {
    Py_RETURN_NONE;
  } else {
    return NULL;
  }
}
