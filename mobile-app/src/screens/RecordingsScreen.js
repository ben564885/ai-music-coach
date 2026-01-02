import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  StyleSheet,
  ScrollView,
  RefreshControl,
  TouchableOpacity,
} from 'react-native';
import {
  Card,
  Text,
  FAB,
  Searchbar,
  Chip,
  Menu,
  IconButton,
  ActivityIndicator,
} from 'react-native-paper';
import { useFocusEffect } from '@react-navigation/native';
import api from '../services/api';
import { format } from 'date-fns';

export default function RecordingsScreen({ navigation }) {
  const [recordings, setRecordings] = useState([]);
  const [filteredRecordings, setFilteredRecordings] = useState([]);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [searchQuery, setSearchQuery] = useState('');
  const [filter, setFilter] = useState('all'); // all, recent, favorites

  const loadRecordings = async () => {
    try {
      const response = await api.get('/api/recordings');
      if (response.data.success) {
        setRecordings(response.data.recordings);
        filterRecordings(response.data.recordings, searchQuery, filter);
      }
    } catch (error) {
      console.error('Error loading recordings:', error);
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  };

  const filterRecordings = (recs, query, filterType) => {
    let filtered = [...recs];

    // Apply search filter
    if (query) {
      filtered = filtered.filter(
        (r) =>
          r.title?.toLowerCase().includes(query.toLowerCase()) ||
          r.feedback?.toLowerCase().includes(query.toLowerCase())
      );
    }

    // Apply type filter
    if (filterType === 'recent') {
      filtered = filtered
        .sort((a, b) => new Date(b.created_at) - new Date(a.created_at))
        .slice(0, 10);
    }

    setFilteredRecordings(filtered);
  };

  useEffect(() => {
    filterRecordings(recordings, searchQuery, filter);
  }, [searchQuery, filter]);

  useFocusEffect(
    useCallback(() => {
      loadRecordings();
    }, [])
  );

  const onRefresh = () => {
    setRefreshing(true);
    loadRecordings();
  };

  const handleDelete = async (recordingId) => {
    try {
      await api.delete(`/api/recordings/${recordingId}`);
      loadRecordings();
    } catch (error) {
      console.error('Error deleting recording:', error);
    }
  };

  const getMistakeCount = (recording) => {
    if (!recording.mistakes) return 0;
    return Array.isArray(recording.mistakes)
      ? recording.mistakes.length
      : 0;
  };

  const formatDate = (dateString) => {
    try {
      return format(new Date(dateString), 'MMM d, yyyy • h:mm a');
    } catch {
      return 'Unknown date';
    }
  };

  if (loading) {
    return (
      <View style={styles.loadingContainer}>
        <ActivityIndicator size="large" />
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Searchbar
          placeholder="Search recordings..."
          onChangeText={setSearchQuery}
          value={searchQuery}
          style={styles.searchbar}
        />
        <View style={styles.filterContainer}>
          <Chip
            selected={filter === 'all'}
            onPress={() => setFilter('all')}
            style={styles.chip}
          >
            All
          </Chip>
          <Chip
            selected={filter === 'recent'}
            onPress={() => setFilter('recent')}
            style={styles.chip}
          >
            Recent
          </Chip>
        </View>
      </View>

      <ScrollView
        style={styles.list}
        refreshControl={
          <RefreshControl refreshing={refreshing} onRefresh={onRefresh} />
        }
      >
        {filteredRecordings.length === 0 ? (
          <View style={styles.emptyContainer}>
            <Text variant="headlineSmall" style={styles.emptyText}>
              {recordings.length === 0
                ? 'No recordings yet'
                : 'No recordings match your search'}
            </Text>
            <Text variant="bodyMedium" style={styles.emptySubtext}>
              {recordings.length === 0
                ? 'Start practicing to see your recordings here'
                : 'Try adjusting your search or filters'}
            </Text>
          </View>
        ) : (
          filteredRecordings.map((recording) => (
            <TouchableOpacity
              key={recording.id}
              onPress={() =>
                navigation.navigate('RecordingDetail', { recording })
              }
            >
              <Card style={styles.card}>
                <Card.Content>
                  <View style={styles.cardHeader}>
                    <View style={styles.cardTitleContainer}>
                      <Text variant="titleMedium" style={styles.cardTitle}>
                        {recording.title || 'Untitled Recording'}
                      </Text>
                      <Text variant="bodySmall" style={styles.cardDate}>
                        {formatDate(recording.created_at)}
                      </Text>
                    </View>
                    <Menu
                      anchor={
                        <IconButton
                          icon="dots-vertical"
                          size={20}
                          onPress={() => {}}
                        />
                      }
                    >
                      <Menu.Item
                        onPress={() => handleDelete(recording.id)}
                        title="Delete"
                        leadingIcon="delete"
                      />
                    </Menu>
                  </View>

                  {recording.feedback && (
                    <Text
                      variant="bodyMedium"
                      style={styles.feedback}
                      numberOfLines={2}
                    >
                      {recording.feedback}
                    </Text>
                  )}

                  <View style={styles.cardFooter}>
                    <Chip
                      icon="music-note"
                      style={styles.mistakeChip}
                      textStyle={styles.mistakeChipText}
                    >
                      {getMistakeCount(recording)} mistakes
                    </Chip>
                    {recording.sheet_music_id && (
                      <Chip icon="file-document" style={styles.sheetChip}>
                        Sheet music
                      </Chip>
                    )}
                  </View>
                </Card.Content>
              </Card>
            </TouchableOpacity>
          ))
        )}
      </ScrollView>

      <FAB
        icon="plus"
        style={styles.fab}
        onPress={() => navigation.navigate('NewRecording')}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  header: {
    padding: 16,
    backgroundColor: '#fff',
    borderBottomWidth: 1,
    borderBottomColor: '#e0e0e0',
  },
  searchbar: {
    marginBottom: 12,
  },
  filterContainer: {
    flexDirection: 'row',
    gap: 8,
  },
  chip: {
    marginRight: 8,
  },
  list: {
    flex: 1,
    padding: 16,
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingTop: 100,
  },
  emptyText: {
    marginBottom: 8,
    color: '#666',
  },
  emptySubtext: {
    color: '#999',
    textAlign: 'center',
  },
  card: {
    marginBottom: 12,
    elevation: 2,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-start',
    marginBottom: 8,
  },
  cardTitleContainer: {
    flex: 1,
  },
  cardTitle: {
    fontWeight: '600',
    marginBottom: 4,
  },
  cardDate: {
    color: '#666',
  },
  feedback: {
    color: '#666',
    marginBottom: 12,
    lineHeight: 20,
  },
  cardFooter: {
    flexDirection: 'row',
    gap: 8,
    flexWrap: 'wrap',
  },
  mistakeChip: {
    backgroundColor: '#ffebee',
  },
  mistakeChipText: {
    color: '#c62828',
  },
  sheetChip: {
    backgroundColor: '#e3f2fd',
  },
  fab: {
    position: 'absolute',
    margin: 16,
    right: 0,
    bottom: 0,
  },
});

