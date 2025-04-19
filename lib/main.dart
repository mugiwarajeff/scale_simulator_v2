import 'package:equipment_simulator_v2/app/app.dart';
import 'package:equipment_simulator_v2/app/scale_simulator/view_model/scale_simulator_view_model.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

void main() {
  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (context) => ScaleSimulatorViewModel()),
      ],

      child: const App(),
    ),
  );
}
