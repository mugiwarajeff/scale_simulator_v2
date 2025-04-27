import 'dart:async';
import 'dart:typed_data';
import "dart:math" as math;
import 'package:equipment_simulator_v2/app/scale_simulator/services/interface/sample_service.dart';
import 'package:equipment_simulator_v2/app/scale_simulator/services/sample_service_impl.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'package:usb_serial/usb_serial.dart';

class ScaleSimulatorViewModel extends ChangeNotifier {
  final SampleService sampleService = SampleServiceImpl();
  bool _isOn = false;

  bool get isOn => _isOn;

  String value = "";
  String? sampleId;

  bool isAutomatic = false;

  int counter = 0;

  final List<UsbDevice> devices = [];

  final List<String> samplesCreatedToday = [];

  TextEditingController valueController = TextEditingController();

  void resetValues() {
    _isOn = false;
    value = "";
    valueController.text = value;
    notifyListeners();
  }

  void setCounter(int number) {
    counter = number;
    notifyListeners();
    print(counter);
  }

  void loadSamples() async {
    samplesCreatedToday.clear();

    samplesCreatedToday.addAll(await sampleService.getSamplesCreatedToday());
    notifyListeners();
  }

  void toggleScale() {
    _isOn = !_isOn;
    value = "";
    valueController.text = value;

    notifyListeners();
  }

  void toggleAutomatic() {
    isAutomatic = !isAutomatic;
    notifyListeners();
  }

  Future<void> getAvailableDevices() async {
    this.devices.clear();
    List<UsbDevice> devices = await UsbSerial.listDevices();

    this.devices.addAll(devices);
    notifyListeners();
  }

  void sendValue(BuildContext context) async {
    if (devices.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text("Connect one device to send weighing"),
          backgroundColor: Colors.red,
        ),
      );

      notifyListeners();
      return;
    }

    if (value != "" && sampleId != null && sampleId!.isNotEmpty) {
      if (devices.isEmpty) {
        notifyListeners();
        return;
      }

      UsbDevice device = devices.first;

      UsbPort? port = await device.create();

      if (port == null) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text("Problems with the USB Port"),
            backgroundColor: Colors.red,
          ),
        );
        notifyListeners();
        return;
      }

      bool openResult = await port.open();

      if (!openResult) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text("Problems with the USB Port"),
            backgroundColor: Colors.red,
          ),
        );
        notifyListeners();
      }

      await port.setPortParameters(
        115200, // baudRate
        UsbPort.DATABITS_8,
        UsbPort.STOPBITS_1,
        UsbPort.PARITY_NONE,
      );

      await port.write(Uint8List.fromList(value.codeUnits));

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text("Weighing sent to print!"),
          backgroundColor: Colors.green,
        ),
      );
      notifyListeners();
      await port.close();

      await _sendSampleWeightToLims();
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text("Weighing or Sample ID are empty!"),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  Future<void> _sendSampleWeightToLims() async {
    if (sampleId == null) {
      return;
    }
    await sampleService.sendSampleWeight(sampleId ?? "", value);
    await sampleService.sendSampleWeightTAGO(sampleId ?? "", value);
  }

  Future<void> generateRandomResult(BuildContext context) async {
    if (!(isOn && isAutomatic)) {
      return;
    }

    math.Random random = math.Random();

    String randomValue = (10 + (random.nextDouble() * (100 - 10)))
        .toStringAsFixed(2);

    valueController.text = randomValue;
    value = randomValue;

    sendValue(context);
  }
}
