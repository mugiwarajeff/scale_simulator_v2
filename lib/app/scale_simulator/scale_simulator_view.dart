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
    widget.scaleSimulatorViewModel.getAvailableDevices().then((_) {
      Timer.periodic(Duration(seconds: 1), (_) async {
        widget.scaleSimulatorViewModel.getAvailableDevices();
      });

      widget.scaleSimulatorViewModel.loadSamples();

      Timer.periodic(Duration(seconds: 1), (timer) {
        widget.scaleSimulatorViewModel.setCounter(
          widget.scaleSimulatorViewModel.counter + 1,
        );

        if (widget.scaleSimulatorViewModel.counter >= 10) {
          widget.scaleSimulatorViewModel.generateRandomResult(context);
          widget.scaleSimulatorViewModel.setCounter(0);
        }
      });
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
              child: Row(
                children: [
                  Image.asset(
                    "assets/labvantage_logo.png",
                    color: onBgColor,
                    width: 400,
                  ),

                  SizedBox(width: 20),
                  Visibility(
                    visible: widget.scaleSimulatorViewModel.isAutomatic,
                    child: ListenableBuilder(
                      listenable: widget.scaleSimulatorViewModel,
                      builder:
                          (context, _) => Text(
                            "Running Automatic... ${widget.scaleSimulatorViewModel.counter}",

                            style: TextStyle(
                              color: Colors.orange,
                              fontSize: 32,
                            ),
                          ),
                    ),
                  ),
                ],
              ),
            ),
            Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text(
                  "Weight",
                  style: TextStyle(color: onBgColor, fontSize: 24),
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
                                  onChanged: (value) {
                                    widget.scaleSimulatorViewModel.value =
                                        value;
                                  },
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
                SizedBox(height: 10),
                ListenableBuilder(
                  listenable: widget.scaleSimulatorViewModel,
                  builder: (context, _) {
                    return Container(
                      decoration: BoxDecoration(
                        color:
                            widget.scaleSimulatorViewModel.isOn
                                ? onBgColor
                                : const Color.fromARGB(255, 68, 68, 68),
                        border: Border.all(color: Colors.grey, width: 10),
                      ),
                      child: SizedBox(
                        width: size.width * 0.68,
                        height: size.height * 0.15,

                        child: Padding(
                          padding: const EdgeInsets.all(8.0),
                          child: DropdownButtonFormField(
                            decoration: InputDecoration(
                              border: InputBorder.none,
                              enabled: widget.scaleSimulatorViewModel.isOn,
                            ),
                            style: TextStyle(fontSize: 32, color: Colors.black),

                            alignment: Alignment.center,
                            items:
                                widget
                                    .scaleSimulatorViewModel
                                    .samplesCreatedToday
                                    .map(
                                      (sample) => DropdownMenuItem(
                                        value: sample,
                                        child: Text(
                                          sample,
                                          textAlign: TextAlign.end,
                                          style: TextStyle(color: Colors.black),
                                        ),
                                      ),
                                    )
                                    .toList(),
                            value: widget.scaleSimulatorViewModel.sampleId,
                            onChanged: (value) {
                              widget.scaleSimulatorViewModel.sampleId = value;
                            },
                          ),
                        ),
                      ),
                    );
                  },
                ),
                SizedBox(height: 10),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text("Min: 10g", style: TextStyle(color: onBgColor)),
                    SizedBox(width: 5),
                    Text("Max: 100g", style: TextStyle(color: onBgColor)),
                  ],
                ),
                SizedBox(height: 20),
                OverflowBar(
                  children: [
                    SizedBox(
                      height: 150,
                      width: 150,
                      child: FloatingActionButton(
                        onPressed: () {
                          widget.scaleSimulatorViewModel.toggleScale();
                        },
                        backgroundColor: Colors.red,

                        child:
                            widget.scaleSimulatorViewModel.isOn
                                ? Text(
                                  "Off",
                                  style: TextStyle(
                                    color: onBgColor,
                                    fontSize: 48,
                                  ),
                                )
                                : Text(
                                  "On",
                                  style: TextStyle(
                                    color: onBgColor,
                                    fontSize: 48,
                                  ),
                                ),
                      ),
                    ),
                    SizedBox(width: 10),
                    ListenableBuilder(
                      listenable: widget.scaleSimulatorViewModel,
                      builder: (context, _) {
                        return SizedBox(
                          height: 150,
                          width: 150,
                          child: FloatingActionButton(
                            onPressed:
                                (widget
                                            .scaleSimulatorViewModel
                                            .devices
                                            .isEmpty ||
                                        widget
                                            .scaleSimulatorViewModel
                                            .isAutomatic)
                                    ? null
                                    : () {
                                      widget.scaleSimulatorViewModel.sendValue(
                                        context,
                                      );
                                    },
                            backgroundColor: onBgColor,
                            child:
                                (widget
                                            .scaleSimulatorViewModel
                                            .devices
                                            .isEmpty ||
                                        widget
                                            .scaleSimulatorViewModel
                                            .isAutomatic)
                                    ? Icon(Icons.lock, size: 84)
                                    : Text(
                                      "Print",
                                      style: TextStyle(
                                        color: bgColor,
                                        fontSize: 32,
                                      ),
                                    ),
                          ),
                        );
                      },
                    ),
                    SizedBox(width: 10),
                    ListenableBuilder(
                      listenable: widget.scaleSimulatorViewModel,
                      builder: (context, _) {
                        return SizedBox(
                          height: 150,
                          width: 150,
                          child: FloatingActionButton(
                            onPressed:
                                !widget.scaleSimulatorViewModel.isOn
                                    ? null
                                    : () {
                                      widget.scaleSimulatorViewModel
                                          .toggleAutomatic();
                                    },
                            backgroundColor: onBgColor,
                            child:
                                !widget.scaleSimulatorViewModel.isOn
                                    ? Icon(Icons.lock, size: 84)
                                    : widget.scaleSimulatorViewModel.isAutomatic
                                    ? Icon(Icons.stop, size: 84)
                                    : Icon(Icons.play_arrow, size: 84),
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
