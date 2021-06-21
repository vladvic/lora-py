/***************************************************
 * pyconn.cpp
 * Created on Thu, 30 Jul 2020 06:47:35 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <iostream>
#include <Python.h>
#include "pyconn.h"

static PyThreadState *_save; 
PyObject *
deviceSessionToPyObject(const lora::DeviceSession *info, bool deep = true);
void
pyObjectToDeviceSession(PyObject *item, lora::DeviceSession *info, bool deep = true);

DataSendCallback dataSendCallback = NULL;

class PythonThreadLock {
public:
  PythonThreadLock() {
    PyEval_RestoreThread(_save);
  }
  ~PythonThreadLock() {
    _save = PyEval_SaveThread();
  }
};

void
lorawan_createDeviceSession(PyObject *self)
{
  PyObject *bases = PyTuple_Pack(0);
  PyObject *emptyString = PyUnicode_FromStringAndSize("", 0);
  PyObject *emptyList = PyList_New(0);
  PyObject *modname = PyModule_GetNameObject(self);
  PyObject *zero = PyLong_FromLong(0);
  PyObject *dict = PyDict_New();
  PyObject *doc = PyUnicode_FromString("DeviceSession class for exchanging device session objects to and from python");

  PyDict_SetItemString(dict, "device",  Py_None);
  PyDict_SetItemString(dict, "networkId",  zero);
  PyDict_SetItemString(dict, "deviceAddr", zero);
  PyDict_SetItemString(dict, "nOnce", zero);
  PyDict_SetItemString(dict, "devNOnce", zero);
  PyDict_SetItemString(dict, "fNwkSIntKey", emptyString);
  PyDict_SetItemString(dict, "sNwkSIntKey", emptyString);
  PyDict_SetItemString(dict, "nwkSEncKey", emptyString);
  PyDict_SetItemString(dict, "appSKey", emptyString);
  PyDict_SetItemString(dict, "lastAccessTime", zero);
  PyDict_SetItemString(dict, "fCntUp", zero);
  PyDict_SetItemString(dict, "fCntDown", zero);
  PyDict_SetItemString(dict, "rxDelay1", zero);
  PyDict_SetItemString(dict, "joinAckDelay1", zero);
  PyDict_SetItemString(dict, "rx2Channel", zero);
  PyDict_SetItemString(dict, "rx2Datarate", zero);
  PyDict_SetItemString(dict, "userInfo", Py_None);

  PyObject *deviceSession = PyObject_CallFunction((PyObject*)&PyType_Type, "sOO",
                                                "DeviceSession", bases, dict);
  PyObject_SetAttrString(deviceSession, "__module__", modname);
  PyObject_SetAttrString(deviceSession, "__doc__", doc);
  PyModule_AddObject(self, "DeviceSession", deviceSession);

  Py_XDECREF(doc);
  Py_XDECREF(emptyString);
  Py_XDECREF(emptyList);
  Py_XDECREF(zero);
  Py_XDECREF(bases);
  Py_XDECREF(modname);
  Py_XDECREF(dict);
  Py_XDECREF(deviceSession);
}

void
lorawan_createDeviceInfo(PyObject *self)
{
  PyObject *bases = PyTuple_Pack(0);
  PyObject *emptyString = PyUnicode_FromStringAndSize("", 0);
  PyObject *emptyList = PyList_New(0);
  PyObject *modname = PyModule_GetNameObject(self);
  PyObject *zero = PyLong_FromLong(0);
  PyObject *dict = PyDict_New();
  PyObject *doc = PyUnicode_FromString("DeviceInfo class for exchanging device info objects to and from python");

  PyDict_SetItemString(dict, "devEUI", emptyString);
  PyDict_SetItemString(dict, "appEUI", emptyString);
  PyDict_SetItemString(dict, "appKey", emptyString);
  PyDict_SetItemString(dict, "cflist", emptyList);
  PyDict_SetItemString(dict, "devClass", zero);
  PyDict_SetItemString(dict, "session", Py_None);

  PyObject *deviceInfo = PyObject_CallFunction((PyObject*)&PyType_Type, "sOO",
                                                "DeviceInfo", bases, dict);
  PyObject_SetAttrString(deviceInfo, "__module__", modname);
  PyObject_SetAttrString(deviceInfo, "__doc__", doc);
  PyModule_AddObject(self, "DeviceInfo", deviceInfo);

  Py_XDECREF(doc);
  Py_XDECREF(emptyString);
  Py_XDECREF(emptyList);
  Py_XDECREF(zero);
  Py_XDECREF(bases);
  Py_XDECREF(modname);
  Py_XDECREF(dict);
  Py_XDECREF(deviceInfo);
}

int
lorawan_initModule(PyObject *self)
{
  lorawan_createDeviceInfo(self);
  lorawan_createDeviceSession(self);

  using namespace lora;
  PyModule_AddIntMacro(self, CLASS_A);
  PyModule_AddIntMacro(self, CLASS_B);
  PyModule_AddIntMacro(self, CLASS_C);

  PyModule_AddIntMacro(self, DR0);
  PyModule_AddIntMacro(self, DR1);
  PyModule_AddIntMacro(self, DR2);
  PyModule_AddIntMacro(self, DR3);
  PyModule_AddIntMacro(self, DR4);
  PyModule_AddIntMacro(self, DR5);
  PyModule_AddIntMacro(self, DR6);
  PyModule_AddIntMacro(self, DR7);
  PyModule_AddIntMacro(self, DR8);
  PyModule_AddIntMacro(self, DR9);
  PyModule_AddIntMacro(self, DR10);
  PyModule_AddIntMacro(self, DR11);
  PyModule_AddIntMacro(self, DR12);
  PyModule_AddIntMacro(self, DR13);
  PyModule_AddIntMacro(self, DR14);
  PyModule_AddIntMacro(self, DR15);

  PyModule_AddIntMacro(self, CR4_5);
  PyModule_AddIntMacro(self, CR4_6);
  PyModule_AddIntMacro(self, CR4_7);
  PyModule_AddIntMacro(self, CR4_8);

  return 0;
}

static PyObject*
lorawan_send(PyObject *self, PyObject *args)
{
  long netid, devaddr, port, confirm;
  PyObject *data;

  if(!PyArg_ParseTuple(args, "lllOl:send", &netid, &devaddr, &port, &data, &confirm)) {
    printf("Failed to parse tuple\n");
    return NULL;
  }

  if(!PyByteArray_Check(data)) {
    printf("Not a bytearray\n");
    return PyLong_FromLong(0);
  }

  if(!dataSendCallback) {
    printf("No data callback\n");
    return PyLong_FromLong(0);
  }

  PyThreadState *old = _save, *save = PyEval_SaveThread();
  dataSendCallback(netid, devaddr, port, 
                          PyByteArray_AsString(data),
                          PyByteArray_Size(data),
                          confirm);
  _save = old;
  PyEval_RestoreThread(save);

  return PyLong_FromLong(PyByteArray_Size(data));
}

static PyMethodDef LorawanMethods[] = {
  {"send", lorawan_send, METH_VARARGS,
   "Send lorawan data."},
  {NULL, NULL, 0, NULL},
};

static PyModuleDef LorawanModule = {
  PyModuleDef_HEAD_INIT, "lorawan", NULL, -1, LorawanMethods,
  NULL, NULL, NULL, NULL,
};

static PyObject*
PyInit_lorawan(void)
{
  PyObject *mod = PyModule_Create(&LorawanModule);
  lorawan_initModule(mod);
  return mod;
}

void
initPythonModule(const std::string &config)
{
  PyImport_AppendInittab("lorawan", &PyInit_lorawan);
  Py_Initialize();
  PyEval_InitThreads();

  PyObject *pName = PyUnicode_DecodeFSDefault(config.c_str());
  PyObject *pModule = PyImport_Import(pName);

  if(!pModule) {
    PyErr_Print();
    std::cerr << "PYTHONPATH=" << getenv("PYTHONPATH") << std::endl;
  }

  Py_XDECREF(pName);
  Py_XDECREF(pModule);

  _save = PyEval_SaveThread();
}

void
finiPythonModule()
{
  PyEval_RestoreThread(_save);

  Py_DECREF(PyImport_ImportModule("threading"));
  Py_FinalizeEx();
}

PyObject *
hexToPyString(const unsigned char *hex, int len) {
  std::string s = lora::hexToString(hex, len);

  return PyUnicode_FromStringAndSize(s.data(), s.size());
}

int
pyStringToHex(PyObject *str, unsigned char *hex, int len) {
  const char *data;
  ssize_t dataSize;

  if(!str) {
    return 0;
  }

  if(PyUnicode_Check(str)) {
    data = PyUnicode_AsUTF8AndSize(str, &dataSize);
    std::string s(data, dataSize);
    return lora::stringToHex(s, hex, len);
  }
  return 0;
}

PyObject *
getType(const char *name) {
  PyObject *pName = PyUnicode_DecodeFSDefault("lorawan");
  PyObject *pModule = PyImport_Import(pName);
  PyObject *pType;

  Py_DECREF(pName);

  pType = PyObject_GetAttrString(pModule, name);

  Py_DECREF(pModule);

  return pType;
}

PyObject *
createObjectFromType(PyObject *pType) {
  if (!pType) return NULL;

  PyObject *pArgs = PyTuple_New(0), *pValue;

  pValue = PyObject_CallObject(pType, pArgs);

  Py_DECREF(pArgs);

  return pValue;
}

PyObject *
deviceInfoToPyObject(const lora::DeviceInfo *info, bool deep = true) {
  PyObject *pType = getType("DeviceInfo");
  PyObject *result = createObjectFromType(pType);
  Py_XDECREF(pType);

  SET_ATTR_FROM_HEX(result, *info, devEUI);
  SET_ATTR_FROM_HEX(result, *info, appEUI);
  SET_ATTR_FROM_HEX(result, *info, appKey);
  SET_ATTR(result, *info, devClass);

  PyObject *cflist = PyList_New(info->cflist.size());

  for(unsigned i = 0; i < info->cflist.size(); i ++) {
    PyObject *item = PyLong_FromLong(info->cflist[i]);
    PyList_SetItem(cflist, i, item);
    Py_XDECREF(item);
  }

  PyObject_SetAttrString(result, "cflist", cflist);

  if (info->session && deep) {
    PyObject *session = deviceSessionToPyObject(info->session, false);
    PyObject_SetAttrString(result, "device", session);
    Py_XDECREF(session);
  }
  else {
    PyObject_SetAttrString(result, "device", Py_None);
  }

  Py_XDECREF(cflist);
  return result;
}

void
pyObjectToDeviceInfo(PyObject *item, lora::DeviceInfo *info, bool deep = true) {
  GET_ATTR(item, devEUI);
  GET_ATTR(item, appEUI);
  GET_ATTR(item, appKey);
  GET_ATTR(item, cflist);
  GET_ATTR(item, devClass);
  GET_ATTR(item, session);

  PY_TO_HEX(*info, devEUI);
  PY_TO_HEX(*info, appEUI);
  PY_TO_HEX(*info, appKey);
  PY_TO_ENUM(*info, devClass, lora::DeviceClass);

  if(PyList_Check(cflist)) {
    for(unsigned i = 0; i < PyList_GET_SIZE(cflist); i ++) {
      PyObject *channel = PyList_GET_ITEM(cflist, i);
      info->cflist.push_back(PyLong_AsLong(channel));
    }
  }

  info->session = NULL;
  if (deep && (session != Py_None)) {
    info->session = new lora::DeviceSession;
    pyObjectToDeviceSession(session, info->session, false);
  }

  Py_XDECREF(devEUI);
  Py_XDECREF(appEUI);
  Py_XDECREF(appKey);
  Py_XDECREF(cflist);
  Py_XDECREF(devClass);
  Py_XDECREF(session);
}

PyObject *
deviceSessionToPyObject(const lora::DeviceSession *info, bool deep) {
  PyObject *pType = getType("DeviceSession");
  PyObject *result = createObjectFromType(pType);
  Py_XDECREF(pType);

  if (info->device && deep) {
    PyObject *device = deviceInfoToPyObject(info->device, false);
    PyObject_SetAttrString(result, "device", device);
    Py_XDECREF(device);
  }
  else {
    PyObject_SetAttrString(result, "device", Py_None);
  }

  SET_ATTR(result, *info, networkId);
  SET_ATTR(result, *info, deviceAddr);
  SET_ATTR(result, *info, nOnce);
  SET_ATTR(result, *info, devNOnce);
  SET_ATTR_FROM_HEX(result, *info, fNwkSIntKey);
  SET_ATTR_FROM_HEX(result, *info, sNwkSIntKey);
  SET_ATTR_FROM_HEX(result, *info, nwkSEncKey);
  SET_ATTR_FROM_HEX(result, *info, appSKey);
  SET_ATTR(result, *info, lastAccessTime);
  SET_ATTR(result, *info, fCntUp);
  SET_ATTR(result, *info, fCntDown);
  SET_ATTR(result, *info, rxDelay1);
  SET_ATTR(result, *info, joinAckDelay1);
  SET_ATTR(result, *info, rx2Channel);
  SET_ATTR(result, *info, rx2Datarate);

  return result;
}

void
pyObjectToDeviceSession(PyObject *item, lora::DeviceSession *info, bool deep) {
  GET_ATTR(item, device);
  GET_ATTR(item, networkId);
  GET_ATTR(item, deviceAddr);
  GET_ATTR(item, nOnce);
  GET_ATTR(item, devNOnce);
  GET_ATTR(item, fNwkSIntKey);
  GET_ATTR(item, sNwkSIntKey);
  GET_ATTR(item, nwkSEncKey);
  GET_ATTR(item, appSKey);
  GET_ATTR(item, lastAccessTime);
  GET_ATTR(item, fCntUp);
  GET_ATTR(item, fCntDown);
  GET_ATTR(item, rxDelay1);
  GET_ATTR(item, joinAckDelay1);
  GET_ATTR(item, rx2Channel);
  GET_ATTR(item, rx2Datarate);

  info->device = NULL;
  if (deep && (device != Py_None)) {
    info->device = new lora::DeviceInfo;
    pyObjectToDeviceInfo(device, info->device, false);
  }

  PY_TO_INT(*info, networkId);
  PY_TO_INT(*info, deviceAddr);
  PY_TO_INT(*info, nOnce);
  PY_TO_INT(*info, devNOnce);
  PY_TO_HEX(*info, fNwkSIntKey);
  PY_TO_HEX(*info, sNwkSIntKey);
  PY_TO_HEX(*info, nwkSEncKey);
  PY_TO_HEX(*info, appSKey);
  PY_TO_INT(*info, lastAccessTime);
  PY_TO_INT(*info, fCntUp);
  PY_TO_INT(*info, fCntDown);
  PY_TO_INT(*info, rxDelay1);
  PY_TO_INT(*info, joinAckDelay1);
  PY_TO_INT(*info, rx2Channel);
  PY_TO_ENUM(*info, rx2Datarate, lora::DataRate);

  Py_XDECREF(device);
  Py_XDECREF(networkId);
  Py_XDECREF(deviceAddr);
  Py_XDECREF(nOnce);
  Py_XDECREF(devNOnce);
  Py_XDECREF(fNwkSIntKey);
  Py_XDECREF(sNwkSIntKey);
  Py_XDECREF(nwkSEncKey);
  Py_XDECREF(appSKey);
  Py_XDECREF(lastAccessTime);
  Py_XDECREF(fCntUp);
  Py_XDECREF(fCntDown);
  Py_XDECREF(rxDelay1);
  Py_XDECREF(joinAckDelay1);
  Py_XDECREF(rx2Channel);
  Py_XDECREF(rx2Datarate);
}

void
processAppData(const std::string &parser, const lora::DeviceSession *s, int port, const char *data, int dataSize)
{
  PythonThreadLock lock;
  const char functionName[] = "ndpd_process_data";
  PyObject *pName, *pModule, *pFunc;
  PyObject *pArgs, *pValue;

  pName = PyUnicode_DecodeFSDefault(parser.c_str());

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if(pModule != NULL) {
    pFunc = PyObject_GetAttrString(pModule, functionName);

    if(pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(3);

      pValue = deviceSessionToPyObject(s);
      PyTuple_SetItem(pArgs, 0, pValue);
      //Py_DECREF(pValue);

      pValue = PyLong_FromLong(port);
      PyTuple_SetItem(pArgs, 1, pValue);
      //Py_DECREF(pValue);

      pValue = PyByteArray_FromStringAndSize(data, dataSize);
      PyTuple_SetItem(pArgs, 2, pValue);
      //Py_DECREF(pValue);

      PyObject_CallObject(pFunc, pArgs);

      Py_DECREF(pArgs);
      Py_XDECREF(pFunc);
    }

    Py_DECREF(pModule);
  }
  else {
    PyErr_Print();
    std::cerr << "PYTHONPATH=" << getenv("PYTHONPATH") << std::endl;
  }
}

void
saveDeviceSession(const std::string &parser, const lora::DeviceSession *s)
{
  PythonThreadLock lock;
  const char functionName[] = "ndpd_save_device_session";
  PyObject *pName, *pModule, *pFunc;
  PyObject *pArgs, *pValue;

  pName = PyUnicode_DecodeFSDefault(parser.c_str());

  pModule = PyImport_Import(pName);

  Py_DECREF(pName);

  if(pModule != NULL) {
    pFunc = PyObject_GetAttrString(pModule, functionName);

    if(pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(1);

      pValue = deviceSessionToPyObject(s);
      PyTuple_SetItem(pArgs, 0, pValue);

      pValue = PyObject_CallObject(pFunc, pArgs);

      if(!pValue) {
        PyErr_Print();
      }

      Py_XDECREF(pArgs);
      Py_XDECREF(pFunc);
      Py_XDECREF(pValue);
    }
    else {
      std::cerr << "Function is not callable!" << std::endl;
    }

    Py_XDECREF(pModule);
  }
  else {
    PyErr_Print();
    std::cerr << "PYTHONPATH=" << getenv("PYTHONPATH") << std::endl;
  }
}

std::list<lora::DeviceSession>
getDeviceSession(const std::string &parser, uint32_t deviceAddr, uint32_t networkAddr)
{
  PythonThreadLock lock;
  std::list<lora::DeviceSession> result;
  const char functionName[] = "ndpd_get_device_session";
  PyObject *pName, *pModule, *pFunc;
  PyObject *pArgs, *pValue;

  pName = PyUnicode_DecodeFSDefault(parser.c_str());

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if(pModule != NULL) {
    pFunc = PyObject_GetAttrString(pModule, functionName);

    if(pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(2);

      pValue = PyLong_FromLong(deviceAddr);
      PyTuple_SetItem(pArgs, 0, pValue);
      //Py_DECREF(pValue);

      pValue = PyLong_FromLong(networkAddr);
      PyTuple_SetItem(pArgs, 1, pValue);
      //Py_DECREF(pValue);

      pValue = PyObject_CallObject(pFunc, pArgs);

      Py_XDECREF(pArgs);
      Py_XDECREF(pFunc);

      if(!pValue) {
        PyErr_Print();
      }
      else if(PyList_Check(pValue)) {
        for(unsigned i = 0; i < PyList_GET_SIZE(pValue); i ++) {
          PyObject *item = PyList_GET_ITEM(pValue, i);
            
          if(item) {
            result.emplace_back();
            pyObjectToDeviceSession(item, &result.back());
            Py_XDECREF(item);
          }
          else {
            printf("Item is NULL!\n");
          }
        }
      }
      else {
        printf("Return result is not list!\n");
      }

      Py_XDECREF(pValue);
    }
    else {
      std::cerr << "Function is not callable!" << std::endl;
    }

    Py_XDECREF(pModule);
  }
  else {
    PyErr_Print();
    std::cerr << "PYTHONPATH=" << getenv("PYTHONPATH") << std::endl;
  }

  return result;
}

std::list<lora::DeviceInfo>
getDeviceInfo(const std::string &parser, const std::string &devEUI, const std::string &appEUI)
{
  PythonThreadLock lock;
  std::list<lora::DeviceInfo> result;
  const char functionName[] = "ndpd_get_device_info";
  PyObject *pName, *pModule, *pFunc;
  PyObject *pArgs, *pValue;

  pName = PyUnicode_DecodeFSDefault(parser.c_str());

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if(pModule != NULL) {
    pFunc = PyObject_GetAttrString(pModule, functionName);

    if(pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(2);

      pValue = PyUnicode_FromStringAndSize(devEUI.data(), devEUI.size());
      PyTuple_SetItem(pArgs, 0, pValue);
      //Py_DECREF(pValue);

      pValue = PyUnicode_FromStringAndSize(appEUI.data(), appEUI.size());
      PyTuple_SetItem(pArgs, 1, pValue);
      //Py_DECREF(pValue);

      pValue = PyObject_CallObject(pFunc, pArgs);

      Py_XDECREF(pArgs);
      Py_XDECREF(pFunc);

      if(!pValue) {
        PyErr_Print();
      }
      else if(PyList_Check(pValue)) {
        for(unsigned i = 0; i < PyList_GET_SIZE(pValue); i ++) {
          PyObject *item = PyList_GET_ITEM(pValue, i);

          if(item) {
            result.emplace_back();
            pyObjectToDeviceInfo(item, &result.back());
            Py_XDECREF(item);
          }
          else {
            printf("Item is NULL!\n");
          }
        }
      }
      else {
        printf("Return result is not list!\n");
      }

      Py_XDECREF(pValue);
    }
    else {
      std::cerr << "Function is not callable!" << std::endl;
    }

    Py_XDECREF(pModule);
  }
  else {
    PyErr_Print();
    std::cerr << "PYTHONPATH=" << getenv("PYTHONPATH") << std::endl;
  }

  return result;
}

