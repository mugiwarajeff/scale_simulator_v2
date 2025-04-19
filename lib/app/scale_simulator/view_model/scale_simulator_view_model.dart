import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:usb_serial/usb_serial.dart';

class ScaleSimulatorViewModel extends ChangeNotifier {
  bool _isOn = false;

  bool get isOn => _isOn;

  String _value = "";

  set value(String value) => _value = value;

  final List<UsbDevice> devices = [];

  TextEditingController valueController = TextEditingController();

  void resetValues() {
    _isOn = false;
    _value = "";
    valueController.text = _value;
    notifyListeners();
  }

  void toggleScale() {
    _isOn = !_isOn;
    valueController.text = _value;

    notifyListeners();
  }

  void getAvailableDevices() async {
    this.devices.clear();
    List<UsbDevice> devices = await UsbSerial.listDevices();

    this.devices.addAll(devices);
    notifyListeners();
  }

  void sendValue() async {
    if (devices.isEmpty) return;

    if (_value == "") return;

    UsbDevice device = devices.first;
    UsbPort? port = await device.create();
    bool openResult = await port!.open();

    if (!openResult) {
      print("Não foi possível abrir a porta.");
      return;
    }

    await port.setDTR(true);
    await port.setRTS(true);
    await port.setPortParameters(
      115200, // baudRate
      UsbPort.DATABITS_8,
      UsbPort.STOPBITS_1,
      UsbPort.PARITY_NONE,
    );

    await port.write(Uint8List.fromList(_value.codeUnits));
    await port.close();
  }
}
