/***************************************************
 * pyconn.h
 * Created on Thu, 30 Jul 2020 06:44:41 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <functional>
#include <string>
#include <list>
#include "lora/types.h"

#define GET_ATTR(item, name) PyObject *name = PyObject_GetAttrString((item), #name)
#define SET_ATTR(obj, item, name) if(1) { \
                                    PyObject *___arg = PyLong_FromLong((long)(item).name); \
                                    PyObject_SetAttrString(obj, #name, ___arg); \
                                    Py_XDECREF(___arg); }
#define SET_ATTR_FROM_HEX(obj, item, name) if(1) { \
                                             PyObject *___arg = hexToPyString((item).name, sizeof((item).name)); \
                                             PyObject_SetAttrString(obj, #name, ___arg); \
                                             Py_XDECREF(___arg); }
#define PY_TO_HEX(item, field) pyStringToHex(field, (item).field, sizeof((item).field));
#define PY_TO_INT(item, field) (item).field = (field == Py_None ? 0 : PyLong_AsLong(field));
#define PY_TO_ENUM(item, field, enumName) (item).field = (field == Py_None ? (enumName)0 : (enumName)PyLong_AsLong(field));

typedef std::function<void(uint32_t, uint32_t, int, const char*, int, int)> DataSendCallback;
typedef std::function<void(const std::string &, const std::string &)> InvalidateCallback;

extern DataSendCallback   dataSendCallback;
extern InvalidateCallback invalidateCallback;

template<typename T>
void
setDataSendCallback(T callback)
{
  dataSendCallback = callback;
}

template<typename T>
void
setInvalidateCallback(T callback)
{
  invalidateCallback = callback;
}

void initPythonModule(const std::string &config);
void finiPythonModule();

void 
processAppData(const std::string &parser, const lora::DeviceSession *s, int port, const char *data, int dataSize);
std::list<lora::DeviceInfo>
getDeviceInfo(const std::string &parser, const std::string &devEUI, const std::string &appEUI);
std::list<lora::DeviceSession>
getDeviceSession(const std::string &parser, uint32_t deviceAddr, uint32_t networkAddr);
void
saveDeviceSession(const std::string &parser, const lora::DeviceSession *s);

