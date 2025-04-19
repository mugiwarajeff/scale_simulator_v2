import 'package:equipment_simulator_v2/app/scale_simulator/scale_simulator_view.dart';
import 'package:equipment_simulator_v2/app/scale_simulator/view_model/scale_simulator_view_model.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

class App extends StatelessWidget {
  const App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      routes: {
        "/":
            (context) => ScaleSimulatorView(
              scaleSimulatorViewModel: Provider.of<ScaleSimulatorViewModel>(
                context,
              ),
            ),
      },
      initialRoute: "/",
    );
  }
}
