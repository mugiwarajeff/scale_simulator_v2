abstract class SampleService {
  Future<List<String>> getSamplesCreatedToday();

  Future<void> sendSampleWeight(String sampleId, String value);

  Future<void> sendSampleWeightTAGO(String sampleId, String value);
}
