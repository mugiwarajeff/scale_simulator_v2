import 'dart:async';

import 'package:equipment_simulator_v2/app/scale_simulator/view_model/scale_simulator_view_model.dart';
import 'package:flutter/material.dart';

class ScaleSimulatorView extends StatefulWidget {
  final ScaleSimulatorViewModel scaleSimulatorViewModel;
  const ScaleSimulatorView({super.key, required this.scaleSimulatorViewModel});

  @override
  State<StatefulWidget> createState() {
    return _ScaleSimulatorViewState();
  }
}

class _ScaleSimulatorViewState extends State<ScaleSimulatorView> {
  final Color bgColor = Color(0xFF156086);
  final Color onBgColor = Colors.white;

  @override
  void initState() {
    super.initState();

    Timer.periodic(Duration(seconds: 1), (_) {
      widget.scaleSimulatorViewModel.getAvailableDevices();
    });
  }

  @override
  Widget build(BuildContext context) {
    final Size size = MediaQuery.of(context).size;
    return Scaffold(
      body: Center(
        child: Stack(
          children: [
            Padding(
              padding: const EdgeInsets.only(left: 24, top: 50.0),
              child: Image.asset(
                "assets/labvantage_logo.png",
                color: onBgColor,
                width: 200,
              ),
            ),
            Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  "Pego Kg",
                  style: TextStyle(color: onBgColor, fontSize: 18),
                ),
                ListenableBuilder(
                  listenable: widget.scaleSimulatorViewModel,
                  builder:
                      (context, _) => Container(
                        width: size.width * 0.7,
                        height: size.height * 0.15,
                        decoration: BoxDecoration(
                          color:
                              widget.scaleSimulatorViewModel.isOn
                                  ? onBgColor
                                  : const Color.fromARGB(255, 68, 68, 68),
                          border: Border.all(color: Colors.grey, width: 10),
                        ),
                        child: Center(
                          child: ListenableBuilder(
                            listenable: widget.scaleSimulatorViewModel,
                            builder:
                                (context, _) => TextField(
                                  controller:
                                      widget
                                          .scaleSimulatorViewModel
                                          .valueController,
                                  textAlign: TextAlign.end,
                                  decoration: InputDecoration(
                                    border: InputBorder.none,
                                    enabled:
                                        widget.scaleSimulatorViewModel.isOn,
                                    suffix: Padding(
                                      padding: const EdgeInsets.all(8.0),
                                      child: Text(
                                        "g",
                                        style: TextStyle(fontSize: 22),
                                      ),
                                    ),
                                  ),
                                  style: TextStyle(fontSize: 32),
                                ),
                          ),
                        ),
                      ),
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text("Bal. 1", style: TextStyle(color: onBgColor)),
                    SizedBox(width: 5),
                    Text("Max 6 kg", style: TextStyle(color: onBgColor)),
                    SizedBox(width: 5),
                    Text("Min 2 g", style: TextStyle(color: onBgColor)),
                    SizedBox(width: 5),
                    Text("e=d=2 g", style: TextStyle(color: onBgColor)),
                  ],
                ),
                SizedBox(height: 20),
                OverflowBar(
                  children: [
                    FloatingActionButton(
                      onPressed: () {
                        widget.scaleSimulatorViewModel.toggleScale();
                      },
                      backgroundColor: Colors.red,
                      child: Text("On/Off", style: TextStyle(color: onBgColor)),
                    ),
                    SizedBox(width: 10),
                    ListenableBuilder(
                      listenable: widget.scaleSimulatorViewModel,
                      builder: (context, _) {
                        return FloatingActionButton(
                          onPressed:
                              widget.scaleSimulatorViewModel.devices.isEmpty
                                  ? null
                                  : () {
                                    widget.scaleSimulatorViewModel.sendValue();
                                  },
                          backgroundColor: onBgColor,
                          child:
                              widget.scaleSimulatorViewModel.devices.isEmpty
                                  ? Icon(Icons.lock)
                                  : Text(
                                    "Print",
                                    style: TextStyle(color: bgColor),
                                  ),
                        );
                      },
                    ),
                  ],
                ),
              ],
            ),
          ],
        ),
      ),
      backgroundColor: bgColor,
    );
  }
}
