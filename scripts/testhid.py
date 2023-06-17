import hid
import binascii
from threading import Thread

# MAYBE, we can register a callback to be notified about device
# add/remove (https://github.com/pyusb/pyusb/pull/160)
from usb import core
from usb import util


def dec_to_hex(value):
    return "{0:x}".format(value)


def str_to_int(value):
    return int(value, base=16)


def str_to_hex(value):
    return hex(str_to_int(value))


class find_class(object):
    def __init__(self, class_):
        self._class = class_

    def __call__(self, device):
        if device.bDeviceClass == self._class:
            return True

        for cfg in device:
            intf = util.find_descriptor(cfg, bInterfaceClass=self._class)
            if intf is not None:
                return True

        return False


# some info can be gathered by:
#   $ lsusb -v
# or device specific:
#   $ lsusb -d vid:pid -v
supported_devices = {
    'Tipro': {'vendor_id': '1222',
              'devices': [
                  {'name': 'Handset Controller',
                   'product_id': 'facb'}
                   ]
              },
    'Vincent Manoukian': {'vendor_id': '303A',
                      'devices': [
                          {'name': 'FFB Pedal',
                           'product_id': '1001'}
                      ]
                      }
}
devices_to_bind = {}

# we can enumarate with vendor_id and product_id as well, useful after some
# type of hotplug event
for dev in hid.enumerate():
    manufacturer = dev.get('manufacturer_string')
    product = dev.get('product_string')
    print(manufacturer)
    print(product)

    if manufacturer in supported_devices:
        for device in supported_devices[manufacturer]['devices']:
            vendor_id = str_to_int(
                supported_devices[manufacturer]['vendor_id'])

            if product == device['name'] and \
                    dec_to_hex(dev.get('product_id')) == device['product_id']:
                print("-----> found")
                product_id = str_to_int(device['product_id'])

                # 3 == hid, 1 == audio
                usb_device = core.find(find_all=True,
                                       custom_match=find_class(3),
                                       idVendor=vendor_id,
                                       idProduct=product_id)

                # differentiate two devices with same vid:pid: u.bus, u.address
                # https://github.com/pyusb/pyusb/blob/master/docs/tutorial.rst#dealing-with-multiple-identical-devices:
                usb_device = [x for x in usb_device][0]

                # bInterfaceProtocol 0 (0 == None, 1 == Keyboard, 2 == Mouse)
                # iInterface 7?
                for conf in usb_device:
                    for interface in conf:
                        if interface.bInterfaceProtocol == 0 and \
                                interface.bInterfaceNumber == dev.get(
                                    'interface_number'):
                            # this will not happen, as we will call add_device
                            # instead
                            devices_to_bind.setdefault("%s %s" %
                                                       (manufacturer, product),
                                                       []).\
                                append({'path': dev.get('path'),
                                        'packet_size': interface[0].
                                        wMaxPacketSize}
                                       )

print("devices to bind:", devices_to_bind)


# open a device and read it's data
# on linux we can open hidraw directly; check if we can do it on macos as well
def read_device(path, packet_size):
    d = hid.Device(path=path)

    while True:
        # macos keep reading "0000000000000000" (or "0100000000000000") while
        # idle
        data = binascii.hexlify(d.read(packet_size)).decode()

        if data:
            print("Read from %s: %s" % (path, data))

    d.close()


for d in devices_to_bind.keys():
    for h in devices_to_bind[d]:
        Thread(target=read_device, args=(h['path'], h['packet_size'])).start()