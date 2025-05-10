import 'dart:convert';

import 'package:equipment_simulator_v2/app/scale_simulator/services/interface/sample_service.dart';
import 'package:http/http.dart';

class SampleServiceImpl implements SampleService {
  Client _client = Client();
  @override
  Future<List<String>> getSamplesCreatedToday() async {
    String token = "Token d1vO-?ku]-7IEX-EFUU-XEVV-QVU7-Hu";

    Map<String, String> headers = {
      "Content-Type": "application/json",
      "Authorization": token,
    };

    Map<String, dynamic> query = {"queryid": "TodaysSamples"};

    Uri baseUri = Uri.https(
      "testlabotech.brdns.net",
      "/labvantage/rest/sdc/samples",
      query,
    );

    Response response = await _client.get(baseUri, headers: headers);

    if (response.statusCode == 200) {
      List<dynamic> bodyMap = jsonDecode(response.body)["samples"];

      if (bodyMap.isEmpty) {
        return [];
      }

      return bodyMap.map((map) => map["s_sampleid"] as String).toList();
    }

    return [];
  }

  @override
  Future<void> sendSampleWeight(String sampleId, String value) async {
    String token = "Token d1vO-?ku]-7IEX-EFUU-XEVV-QVU7-Hu";

    Map<String, String> headers = {
      "Content-Type": "application/json",
      "Authorization": token,
    };

    Map<String, dynamic> body = {
      "attributeid": "Weight",
      "attributeinstance": 1,
      "keyid1": sampleId,
      "attributesdcid": "Sample",
      "sdcid": "Sample",
      "value": value,
    };

    Uri baseUri = Uri.https(
      "testlabotech.brdns.net",
      "/labvantage/rest/actions/EditSDIAttribute",
    );

    Response response = await _client.post(
      baseUri,
      headers: headers,
      body: jsonEncode(body),
    );

    if (response.statusCode == 200) {
      return;
    }
  }

  @override
  Future<void> sendSampleWeightTAGO(String sampleId, String value) async {
    const String apiUrl = 'https://api.tago.io/data?variable=atuador01&qty=1';
    DateTime dateTime = DateTime.now();

    // Construindo o payload JSON
    final List<Map<String, dynamic>> variables = [
      {"variable": "id", "value": dateTime.toIso8601String()},
      {"variable": "date", "value": dateTime.toString()},
      {"variable": "hour", "value": dateTime.hour.toString()},
      {
        "variable": "textid",
        "value": "Weight-LV-${dateTime.toIso8601String()}",
      },
      {"variable": "analysis", "value": "Weight"},
      {"variable": "result", "value": num.tryParse(value) ?? value},
      {"variable": "unit", "value": 'g'},
      {"variable": "resp", "value": "SYS_IOT"},
      {"variable": "status", "value": "READING OK"},
    ];

    // Headers da requisição
    final Map<String, String> headers = {
      'Device-Token': "8961a44f-ce78-41b4-aa45-8d28caaee714",
      'Content-Type': 'application/json',
    };

    // Fazendo a requisição POST
    final response = await _client.post(
      Uri.parse(apiUrl),
      headers: headers,
      body: jsonEncode(variables),
    );

    // Verificando a resposta
    if (response.statusCode == 200) {
      print('Dados enviados com sucesso para Tago.io');
      print('Resposta: ${response.body}');
    } else {
      print('Erro ao enviar dados para Tago.io');
      print('Status code: ${response.statusCode}');
      print('Resposta: ${response.body}');
    }
  }
}
