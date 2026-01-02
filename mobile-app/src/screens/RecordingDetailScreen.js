import React from 'react';
import { View, StyleSheet, ScrollView } from 'react-native';
import { Card, Text, Chip, Divider } from 'react-native-paper';
import MistakeTimeline from '../components/MistakeTimeline';

export default function RecordingDetailScreen({ route }) {
  const { recording } = route.params;

  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Text variant="headlineSmall" style={styles.title}>
            {recording.title || 'Untitled Recording'}
          </Text>
          <Text variant="bodyMedium" style={styles.date}>
            {new Date(recording.created_at).toLocaleString()}
          </Text>

          {recording.feedback && (
            <>
              <Divider style={styles.divider} />
              <Text variant="titleMedium" style={styles.sectionTitle}>
                AI Coach Feedback
              </Text>
              <Text variant="bodyMedium" style={styles.feedback}>
                {recording.feedback}
              </Text>
            </>
          )}

          <View style={styles.chips}>
            <Chip icon="music-note">
              {recording.mistakes?.length || 0} mistakes detected
            </Chip>
          </View>
        </Card.Content>
      </Card>

      {recording.mistakes && recording.mistakes.length > 0 && (
        <MistakeTimeline
          mistakes={recording.mistakes}
          feedback={recording.feedback ? { text: recording.feedback } : null}
          audioPath={recording.audio_url}
        />
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  card: {
    margin: 16,
  },
  title: {
    marginBottom: 8,
    fontWeight: 'bold',
  },
  date: {
    color: '#666',
    marginBottom: 16,
  },
  divider: {
    marginVertical: 16,
  },
  sectionTitle: {
    marginBottom: 8,
    fontWeight: '600',
  },
  feedback: {
    lineHeight: 22,
    marginBottom: 16,
  },
  chips: {
    flexDirection: 'row',
    gap: 8,
    flexWrap: 'wrap',
  },
});

