import React, { useState } from 'react';
import { StyleSheet, View } from 'react-native';
import { Card, SegmentedButtons, Text } from 'react-native-paper';

const INSTRUMENTS = [
  { value: 'piano', label: 'Piano', icon: 'piano' },
  { value: 'violin', label: 'Violin', icon: 'violin' },
  { value: 'guitar', label: 'Guitar', icon: 'guitar-electric' },
  { value: 'flute', label: 'Flute', icon: 'flute' },
  { value: 'clarinet', label: 'Clarinet', icon: 'clarinet' },
  { value: 'trumpet', label: 'Trumpet', icon: 'trumpet' },
  { value: 'saxophone', label: 'Saxophone', icon: 'saxophone' },
];

export default function InstrumentSelector({ selectedInstrument, onInstrumentChange }) {
  return (
    <Card style={styles.card}>
      <Card.Content>
        <Text variant="titleMedium" style={styles.title}>
          Select Instrument
        </Text>
        <Text variant="bodySmall" style={styles.subtitle}>
          Choose your instrument for fingering guidance
        </Text>
        
        <SegmentedButtons
          value={selectedInstrument}
          onValueChange={onInstrumentChange}
          buttons={INSTRUMENTS.map(inst => ({
            value: inst.value,
            label: inst.label,
          }))}
          style={styles.buttons}
        />
      </Card.Content>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    marginBottom: 16,
  },
  title: {
    marginBottom: 4,
  },
  subtitle: {
    color: '#666',
    marginBottom: 12,
  },
  buttons: {
    marginTop: 8,
  },
});

