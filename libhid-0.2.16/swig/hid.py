# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.40
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.
# This file is compatible with both classic and new-style classes.

"""
libhid is a user-space USB HID access library built on libusb.
"""

from sys import version_info
if version_info >= (2,6,0):
    def swig_import_helper():
        from os.path import dirname
        import imp
        fp = None
        try:
            fp, pathname, description = imp.find_module('_hid', [dirname(__file__)])
        except ImportError:
            import _hid
            return _hid
        if fp is not None:
            try:
                _mod = imp.load_module('_hid', fp, pathname, description)
            finally:
                fp.close()
            return _mod
    _hid = swig_import_helper()
    del swig_import_helper
else:
    import _hid
del version_info
try:
    _swig_property = property
except NameError:
    pass # Python < 2.2 doesn't have 'property'.
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "thisown"): return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'SwigPyObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    if (name == "thisown"): return self.this.own()
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError(name)

def _swig_repr(self):
    try: strthis = "proxy of " + self.this.__repr__()
    except: strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

try:
    _object = object
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0



def wrap_hid_interrupt_read(*args):
  """wrap_hid_interrupt_read(hidif, ep, bytes_out, timeout)"""
  return _hid.wrap_hid_interrupt_read(*args)

def wrap_hid_get_input_report(*args):
  """wrap_hid_get_input_report(hidif, path, bytes_out)"""
  return _hid.wrap_hid_get_input_report(*args)

def wrap_hid_get_feature_report(*args):
  """wrap_hid_get_feature_report(hidif, path, bytes_out)"""
  return _hid.wrap_hid_get_feature_report(*args)
true = _hid.true
false = _hid.false
HID_RET_SUCCESS = _hid.HID_RET_SUCCESS
HID_RET_INVALID_PARAMETER = _hid.HID_RET_INVALID_PARAMETER
HID_RET_NOT_INITIALISED = _hid.HID_RET_NOT_INITIALISED
HID_RET_ALREADY_INITIALISED = _hid.HID_RET_ALREADY_INITIALISED
HID_RET_FAIL_FIND_BUSSES = _hid.HID_RET_FAIL_FIND_BUSSES
HID_RET_FAIL_FIND_DEVICES = _hid.HID_RET_FAIL_FIND_DEVICES
HID_RET_FAIL_OPEN_DEVICE = _hid.HID_RET_FAIL_OPEN_DEVICE
HID_RET_DEVICE_NOT_FOUND = _hid.HID_RET_DEVICE_NOT_FOUND
HID_RET_DEVICE_NOT_OPENED = _hid.HID_RET_DEVICE_NOT_OPENED
HID_RET_DEVICE_ALREADY_OPENED = _hid.HID_RET_DEVICE_ALREADY_OPENED
HID_RET_FAIL_CLOSE_DEVICE = _hid.HID_RET_FAIL_CLOSE_DEVICE
HID_RET_FAIL_CLAIM_IFACE = _hid.HID_RET_FAIL_CLAIM_IFACE
HID_RET_FAIL_DETACH_DRIVER = _hid.HID_RET_FAIL_DETACH_DRIVER
HID_RET_NOT_HID_DEVICE = _hid.HID_RET_NOT_HID_DEVICE
HID_RET_HID_DESC_SHORT = _hid.HID_RET_HID_DESC_SHORT
HID_RET_REPORT_DESC_SHORT = _hid.HID_RET_REPORT_DESC_SHORT
HID_RET_REPORT_DESC_LONG = _hid.HID_RET_REPORT_DESC_LONG
HID_RET_FAIL_ALLOC = _hid.HID_RET_FAIL_ALLOC
HID_RET_OUT_OF_SPACE = _hid.HID_RET_OUT_OF_SPACE
HID_RET_FAIL_SET_REPORT = _hid.HID_RET_FAIL_SET_REPORT
HID_RET_FAIL_GET_REPORT = _hid.HID_RET_FAIL_GET_REPORT
HID_RET_FAIL_INT_READ = _hid.HID_RET_FAIL_INT_READ
HID_RET_NOT_FOUND = _hid.HID_RET_NOT_FOUND
HID_RET_TIMEOUT = _hid.HID_RET_TIMEOUT
class HIDInterface(_object):
    """Proxy of C HIDInterface struct"""
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, HIDInterface, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, HIDInterface, name)
    __repr__ = _swig_repr
    __swig_getmethods__["device"] = _hid.HIDInterface_device_get
    if _newclass:device = _swig_property(_hid.HIDInterface_device_get)
    __swig_getmethods__["interface"] = _hid.HIDInterface_interface_get
    if _newclass:interface = _swig_property(_hid.HIDInterface_interface_get)
    __swig_getmethods__["id"] = _hid.HIDInterface_id_get
    if _newclass:id = _swig_property(_hid.HIDInterface_id_get)
    def __init__(self): 
        """__init__(self) -> HIDInterface"""
        this = _hid.new_HIDInterface()
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _hid.delete_HIDInterface
    __del__ = lambda self : None;
HIDInterface_swigregister = _hid.HIDInterface_swigregister
HIDInterface_swigregister(HIDInterface)

class HIDInterfaceMatcher(_object):
    """Proxy of C HIDInterfaceMatcher struct"""
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, HIDInterfaceMatcher, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, HIDInterfaceMatcher, name)
    __repr__ = _swig_repr
    __swig_setmethods__["vendor_id"] = _hid.HIDInterfaceMatcher_vendor_id_set
    __swig_getmethods__["vendor_id"] = _hid.HIDInterfaceMatcher_vendor_id_get
    if _newclass:vendor_id = _swig_property(_hid.HIDInterfaceMatcher_vendor_id_get, _hid.HIDInterfaceMatcher_vendor_id_set)
    __swig_setmethods__["product_id"] = _hid.HIDInterfaceMatcher_product_id_set
    __swig_getmethods__["product_id"] = _hid.HIDInterfaceMatcher_product_id_get
    if _newclass:product_id = _swig_property(_hid.HIDInterfaceMatcher_product_id_get, _hid.HIDInterfaceMatcher_product_id_set)
    def __init__(self): 
        """__init__(self) -> HIDInterfaceMatcher"""
        this = _hid.new_HIDInterfaceMatcher()
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _hid.delete_HIDInterfaceMatcher
    __del__ = lambda self : None;
HIDInterfaceMatcher_swigregister = _hid.HIDInterfaceMatcher_swigregister
HIDInterfaceMatcher_swigregister(HIDInterfaceMatcher)

HID_ID_MATCH_ANY = _hid.HID_ID_MATCH_ANY
HID_DEBUG_NONE = _hid.HID_DEBUG_NONE
HID_DEBUG_ERRORS = _hid.HID_DEBUG_ERRORS
HID_DEBUG_WARNINGS = _hid.HID_DEBUG_WARNINGS
HID_DEBUG_NOTICES = _hid.HID_DEBUG_NOTICES
HID_DEBUG_TRACES = _hid.HID_DEBUG_TRACES
HID_DEBUG_ASSERTS = _hid.HID_DEBUG_ASSERTS
HID_DEBUG_NOTRACES = _hid.HID_DEBUG_NOTRACES
HID_DEBUG_ALL = _hid.HID_DEBUG_ALL

def hid_set_debug(*args):
  """hid_set_debug(level)"""
  return _hid.hid_set_debug(*args)

def hid_set_debug_stream(*args):
  """hid_set_debug_stream(outstream)"""
  return _hid.hid_set_debug_stream(*args)

def hid_set_usb_debug(*args):
  """hid_set_usb_debug(level)"""
  return _hid.hid_set_usb_debug(*args)

def hid_new_HIDInterface():
  """hid_new_HIDInterface()"""
  return _hid.hid_new_HIDInterface()

def hid_delete_HIDInterface(*args):
  """hid_delete_HIDInterface(hidif)"""
  return _hid.hid_delete_HIDInterface(*args)

def hid_reset_HIDInterface(*args):
  """hid_reset_HIDInterface(hidif)"""
  return _hid.hid_reset_HIDInterface(*args)

def hid_init():
  """hid_init()"""
  return _hid.hid_init()

def hid_cleanup():
  """hid_cleanup()"""
  return _hid.hid_cleanup()

def hid_is_initialised():
  """hid_is_initialised() -> _Bool"""
  return _hid.hid_is_initialised()

def hid_open(*args):
  """hid_open(hidif, interface, matcher)"""
  return _hid.hid_open(*args)

def hid_force_open(*args):
  """hid_force_open(hidif, interface, matcher, retries)"""
  return _hid.hid_force_open(*args)

def hid_close(*args):
  """hid_close(hidif)"""
  return _hid.hid_close(*args)

def hid_is_opened(*args):
  """hid_is_opened(hidif) -> _Bool"""
  return _hid.hid_is_opened(*args)

def hid_strerror(*args):
  """hid_strerror(ret) -> char"""
  return _hid.hid_strerror(*args)

def hid_get_input_report(*args):
  """hid_get_input_report(hidif, path, size) -> hid_return,bytes"""
  return _hid.hid_get_input_report(*args)

def hid_set_output_report(*args):
  """hid_set_output_report(hidif, path, buffer)"""
  return _hid.hid_set_output_report(*args)

def hid_get_feature_report(*args):
  """hid_get_feature_report(hidif, path, size) -> hid_return,bytes"""
  return _hid.hid_get_feature_report(*args)

def hid_set_feature_report(*args):
  """hid_set_feature_report(hidif, path, buffer)"""
  return _hid.hid_set_feature_report(*args)

def hid_get_item_value(*args):
  """hid_get_item_value(hidif, path)"""
  return _hid.hid_get_item_value(*args)

def hid_write_identification(*args):
  """hid_write_identification(out, hidif)"""
  return _hid.hid_write_identification(*args)

def hid_dump_tree(*args):
  """hid_dump_tree(out, hidif)"""
  return _hid.hid_dump_tree(*args)

def hid_interrupt_read(*args):
  """hid_interrupt_read(hidif, ep, size, timeout) -> hid_return,bytes"""
  return _hid.hid_interrupt_read(*args)

def hid_interrupt_write(*args):
  """hid_interrupt_write(hidif, ep, bytes, timeout)"""
  return _hid.hid_interrupt_write(*args)

def hid_set_idle(*args):
  """hid_set_idle(hidif, duration, report_id)"""
  return _hid.hid_set_idle(*args)
_doc = hid_interrupt_read.__doc__
hid_interrupt_read = wrap_hid_interrupt_read
hid_interrupt_read.__doc__ = _doc

_doc = hid_get_input_report.__doc__
hid_get_input_report = wrap_hid_get_input_report
hid_get_input_report.__doc__ = _doc

_doc = hid_get_feature_report.__doc__
hid_get_feature_report = wrap_hid_get_feature_report
hid_get_feature_report.__doc__ = _doc

import sys
hid_return = {}
for sym in dir(sys.modules[__name__]):
    if sym.startswith('HID_RET_'):
        hid_return[eval(sym)] = sym



