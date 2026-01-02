import React, { useState, useRef } from 'react';
import { StyleSheet, View, ScrollView, TouchableOpacity, Dimensions } from 'react-native';
import { Card, Text, Chip, Divider, Button } from 'react-native-paper';
import AudioRecorderPlayer from 'react-native-audio-recorder-player';

const { width } = Dimensions.get('window');

export default function MistakeTimeline({ mistakes, feedback, audioPath }) {
  const [currentTime, setCurrentTime] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const audioPlayer = useRef(new AudioRecorderPlayer());

  const getMistakeColor = (type) => {
    switch (type) {
      case 'note_accuracy':
        return '#f44336'; // Red
      case 'hesitation':
      case 'rushing':
      case 'tempo_deviation':
        return '#ff9800'; // Orange
      case 'dynamics':
        return '#2196f3'; // Blue
      default:
        return '#757575'; // Gray
    }
  };

  const getMistakeIcon = (type) => {
    switch (type) {
      case 'note_accuracy':
        return 'music-note';
      case 'hesitation':
        return 'pause';
      case 'rushing':
        return 'fast-forward';
      case 'tempo_deviation':
        return 'metronome';
      case 'dynamics':
        return 'volume-high';
      default:
        return 'alert';
    }
  };

  const formatTime = (seconds) => {
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
  };

  const getMistakeDescription = (mistake) => {
    switch (mistake.type) {
      case 'note_accuracy':
        return `Expected ${mistake.expected}, played ${mistake.played}`;
      case 'hesitation':
        return `Paused for ${Math.round(mistake.gap_ms)}ms too long`;
      case 'rushing':
        return `Played at ${Math.round(mistake.bpm_detected)} BPM (expected ${Math.round(mistake.bpm_expected)})`;
      case 'tempo_deviation':
        return `Overall tempo: ${Math.round(mistake.detected_tempo)} BPM (expected ${Math.round(mistake.expected_tempo)})`;
      case 'dynamics':
        return `Expected ${mistake.marking.toUpperCase()}, actual: ${Math.round(mistake.actual_db)} dB`;
      default:
        return 'Issue detected';
    }
  };

  const seekToMistake = async (timestamp) => {
    try {
      await audioPlayer.current.seekToPlayer(timestamp * 1000); // Convert to milliseconds
      if (!isPlaying) {
        await audioPlayer.current.startPlayer(audioPath);
        setIsPlaying(true);
      }
    } catch (error) {
      console.error('Error seeking:', error);
    }
  };

  const togglePlayback = async () => {
    try {
      if (isPlaying) {
        await audioPlayer.current.pausePlayer();
        setIsPlaying(false);
      } else {
        await audioPlayer.current.startPlayer(audioPath);
        setIsPlaying(true);
        
        audioPlayer.current.addPlayBackListener((e) => {
          setCurrentTime(e.currentPosition / 1000); // Convert to seconds
          if (e.currentPosition === e.duration) {
            setIsPlaying(false);
          }
        });
      }
    } catch (error) {
      console.error('Error with playback:', error);
    }
  };

  // Group mistakes by type for summary
  const mistakesByType = mistakes.reduce((acc, mistake) => {
    const type = mistake.type;
    if (!acc[type]) acc[type] = [];
    acc[type].push(mistake);
    return acc;
  }, {});

  return (
    <ScrollView style={styles.container}>
      {/* Feedback Card */}
      {feedback && (
        <Card style={styles.feedbackCard}>
          <Card.Content>
            <Text variant="titleMedium" style={styles.feedbackTitle}>
              AI Coach Feedback
            </Text>
            <Text variant="bodyMedium" style={styles.feedbackText}>
              {feedback.text}
            </Text>
            {feedback.suggestions && feedback.suggestions.length > 0 && (
              <View style={styles.suggestionsContainer}>
                <Text variant="labelMedium" style={styles.suggestionsTitle}>
                  Action Items:
                </Text>
                {feedback.suggestions.map((suggestion, idx) => (
                  <Text key={idx} variant="bodySmall" style={styles.suggestion}>
                    • {suggestion.message}
                  </Text>
                ))}
              </View>
            )}
          </Card.Content>
        </Card>
      )}

      {/* Summary Stats */}
      <Card style={styles.summaryCard}>
        <Card.Content>
          <Text variant="titleMedium" style={styles.summaryTitle}>
            Performance Summary
          </Text>
          <View style={styles.statsContainer}>
            <View style={styles.stat}>
              <Text variant="headlineSmall" style={styles.statNumber}>
                {mistakes.length}
              </Text>
              <Text variant="bodySmall">Total Issues</Text>
            </View>
            {Object.entries(mistakesByType).map(([type, items]) => (
              <View key={type} style={styles.stat}>
                <Text variant="headlineSmall" style={[styles.statNumber, { color: getMistakeColor(type) }]}>
                  {items.length}
                </Text>
                <Text variant="bodySmall">{type.replace('_', ' ')}</Text>
              </View>
            ))}
          </View>
        </Card.Content>
      </Card>

      {/* Audio Player */}
      {audioPath && (
        <Card style={styles.playerCard}>
          <Card.Content>
            <View style={styles.playerControls}>
              <Button
                mode="contained"
                icon={isPlaying ? 'pause' : 'play'}
                onPress={togglePlayback}
              >
                {isPlaying ? 'Pause' : 'Play'}
              </Button>
              <Text variant="bodySmall" style={styles.timeDisplay}>
                {formatTime(currentTime)}
              </Text>
            </View>
          </Card.Content>
        </Card>
      )}

      {/* Mistake List */}
      <Card style={styles.mistakesCard}>
        <Card.Content>
          <Text variant="titleMedium" style={styles.mistakesTitle}>
            Detailed Mistakes
          </Text>
          <Divider style={styles.divider} />
          
          {mistakes.map((mistake, idx) => (
            <TouchableOpacity
              key={idx}
              onPress={() => seekToMistake(mistake.timestamp)}
              style={styles.mistakeItem}
            >
              <View style={styles.mistakeHeader}>
                <Chip
                  icon={getMistakeIcon(mistake.type)}
                  style={[styles.mistakeChip, { backgroundColor: getMistakeColor(mistake.type) + '20' }]}
                  textStyle={{ color: getMistakeColor(mistake.type) }}
                >
                  {mistake.type.replace('_', ' ')}
                </Chip>
                <Text variant="bodySmall" style={styles.timestamp}>
                  {formatTime(mistake.timestamp)}
                </Text>
              </View>
              
              <Text variant="bodyMedium" style={styles.mistakeDescription}>
                {getMistakeDescription(mistake)}
              </Text>
              
              {mistake.measure && (
                <Text variant="bodySmall" style={styles.measureInfo}>
                  Measure {mistake.measure}
                  {mistake.beat && `, Beat ${mistake.beat}`}
                </Text>
              )}
              
              {idx < mistakes.length - 1 && <Divider style={styles.itemDivider} />}
            </TouchableOpacity>
          ))}
        </Card.Content>
      </Card>

      {/* Save Button */}
      <Button
        mode="contained"
        icon="download"
        style={styles.saveButton}
        onPress={() => {
          // TODO: Implement save annotated PDF functionality
          console.log('Save annotated sheet music');
        }}
      >
        Save Marked Sheet Music
      </Button>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 16,
  },
  feedbackCard: {
    marginBottom: 16,
    backgroundColor: '#e3f2fd',
  },
  feedbackTitle: {
    marginBottom: 12,
    fontWeight: 'bold',
  },
  feedbackText: {
    marginBottom: 12,
    lineHeight: 22,
  },
  suggestionsContainer: {
    marginTop: 12,
    paddingTop: 12,
    borderTopWidth: 1,
    borderTopColor: '#ccc',
  },
  suggestionsTitle: {
    marginBottom: 8,
    fontWeight: 'bold',
  },
  suggestion: {
    marginBottom: 4,
  },
  summaryCard: {
    marginBottom: 16,
  },
  summaryTitle: {
    marginBottom: 16,
    fontWeight: 'bold',
  },
  statsContainer: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    flexWrap: 'wrap',
  },
  stat: {
    alignItems: 'center',
    marginBottom: 12,
    minWidth: 80,
  },
  statNumber: {
    fontWeight: 'bold',
    marginBottom: 4,
  },
  playerCard: {
    marginBottom: 16,
  },
  playerControls: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  timeDisplay: {
    marginLeft: 16,
  },
  mistakesCard: {
    marginBottom: 16,
  },
  mistakesTitle: {
    marginBottom: 12,
    fontWeight: 'bold',
  },
  divider: {
    marginBottom: 16,
  },
  mistakeItem: {
    paddingVertical: 12,
  },
  mistakeHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  mistakeChip: {
    marginRight: 8,
  },
  timestamp: {
    color: '#666',
  },
  mistakeDescription: {
    marginBottom: 4,
  },
  measureInfo: {
    color: '#666',
    fontStyle: 'italic',
  },
  itemDivider: {
    marginTop: 12,
  },
  saveButton: {
    marginTop: 16,
    marginBottom: 32,
  },
});

