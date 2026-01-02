import React, { useState } from 'react';
import { View, StyleSheet, ScrollView } from 'react-native';
import { Text, Card, Switch } from 'react-native-paper';
import SheetMusicUpload from '../components/SheetMusicUpload';
import DeviceConnection from '../components/DeviceConnection';
import InstrumentSelector from '../components/InstrumentSelector';

export default function NewRecordingScreen({ navigation }) {
  const [selectedInstrument, setSelectedInstrument] = useState('piano');
  const [realTimeFeedback, setRealTimeFeedback] = useState(true);

  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Text variant="headlineSmall" style={styles.title}>
            Start New Recording
          </Text>
          <Text variant="bodyMedium" style={styles.subtitle}>
            Upload sheet music and connect your T5AI device to begin
          </Text>
        </Card.Content>
      </Card>

      <InstrumentSelector
        selectedInstrument={selectedInstrument}
        onInstrumentChange={setSelectedInstrument}
      />

      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.switchRow}>
            <View style={styles.switchText}>
              <Text variant="titleMedium">Real-Time Feedback</Text>
              <Text variant="bodySmall" style={styles.switchSubtext}>
                Get immediate note detection and fingering guidance on T5AI screen
              </Text>
            </View>
            <Switch
              value={realTimeFeedback}
              onValueChange={setRealTimeFeedback}
            />
          </View>
        </Card.Content>
      </Card>

      <DeviceConnection />
      <SheetMusicUpload />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    padding: 16,
  },
  card: {
    marginBottom: 16,
  },
  title: {
    marginBottom: 8,
    fontWeight: 'bold',
  },
  subtitle: {
    color: '#666',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  switchText: {
    flex: 1,
    marginRight: 16,
  },
  switchSubtext: {
    color: '#666',
    marginTop: 4,
  },
});

