import React, { useState } from 'react';
import { StyleSheet, View, Alert } from 'react-native';
import { Card, Button, Text, ProgressBar, ActivityIndicator } from 'react-native-paper';
import * as DocumentPicker from 'expo-document-picker';
import * as ImagePicker from 'expo-image-picker';
import { API_BASE_URL, SUPPORTED_IMAGE_FORMATS, SUPPORTED_DOCUMENT_FORMATS } from '../config';
import axios from 'axios';

export default function SheetMusicUpload({ onUploadComplete, sheetMusicData }) {
  const [uploading, setUploading] = useState(false);
  const [uploadProgress, setUploadProgress] = useState(0);

  const pickImage = async () => {
    try {
      const result = await ImagePicker.launchImageLibraryAsync({
        mediaTypes: ImagePicker.MediaTypeOptions.Images,
        allowsEditing: true,
        quality: 0.8,
      });

      if (!result.canceled && result.assets[0]) {
        await uploadFile(result.assets[0].uri, 'image');
      }
    } catch (error) {
      Alert.alert('Error', 'Failed to pick image: ' + error.message);
    }
  };

  const pickDocument = async () => {
    try {
      const result = await DocumentPicker.getDocumentAsync({
        type: ['application/pdf', 'application/xml', 'text/xml'],
        copyToCacheDirectory: true,
      });

      if (!result.canceled && result.assets[0]) {
        await uploadFile(result.assets[0].uri, 'document');
      }
    } catch (error) {
      Alert.alert('Error', 'Failed to pick document: ' + error.message);
    }
  };

  const uploadFile = async (fileUri, fileType) => {
    setUploading(true);
    setUploadProgress(0);

    try {
      const formData = new FormData();
      formData.append('file', {
        uri: fileUri,
        type: fileType === 'image' ? 'image/jpeg' : 'application/pdf',
        name: fileType === 'image' ? 'sheet_music.jpg' : 'sheet_music.pdf',
      });

      const response = await axios.post(
        `${API_BASE_URL}/api/upload-sheet-music`,
        formData,
        {
          headers: {
            'Content-Type': 'multipart/form-data',
          },
          onUploadProgress: (progressEvent) => {
            const progress = progressEvent.loaded / progressEvent.total;
            setUploadProgress(progress);
          },
        }
      );

      if (response.data.status === 'uploaded') {
        // If OMR is not implemented, prompt for MusicXML
        if (response.data.message.includes('OMR processing not yet implemented')) {
          Alert.alert(
            'Manual Input Required',
            'Please provide MusicXML or MIDI data directly. You can paste MusicXML or upload a MIDI file.',
            [
              { text: 'Cancel', style: 'cancel' },
              { text: 'Enter MusicXML', onPress: () => promptMusicXML() },
            ]
          );
        } else {
          onUploadComplete(response.data);
        }
      }
    } catch (error) {
      Alert.alert('Upload Error', error.response?.data?.error || error.message);
    } finally {
      setUploading(false);
      setUploadProgress(0);
    }
  };

  const promptMusicXML = () => {
    // For now, show alert. In production, use a modal with text input
    Alert.alert(
      'MusicXML Input',
      'MusicXML input interface coming soon. For now, use the API endpoint directly.',
      [{ text: 'OK' }]
    );
  };

  const processMusicXML = async (musicxmlContent) => {
    setUploading(true);
    try {
      const response = await axios.post(`${API_BASE_URL}/api/process-musicxml`, {
        musicxml: musicxmlContent,
      });

      if (response.data.success) {
        onUploadComplete(response.data.reference_data);
      }
    } catch (error) {
      Alert.alert('Error', error.response?.data?.error || error.message);
    } finally {
      setUploading(false);
    }
  };

  return (
    <Card style={styles.card}>
      <Card.Content>
        <Text variant="titleMedium" style={styles.title}>
          Upload Sheet Music
        </Text>
        <Text variant="bodyMedium" style={styles.subtitle}>
          Take a photo or upload a PDF/MusicXML file
        </Text>

        {uploading && (
          <View style={styles.progressContainer}>
            <ProgressBar progress={uploadProgress} color="#6200ee" />
            <ActivityIndicator style={styles.loader} />
          </View>
        )}

        {sheetMusicData && (
          <View style={styles.successContainer}>
            <Text variant="bodySmall" style={styles.successText}>
              ✓ Sheet music loaded successfully
            </Text>
          </View>
        )}

        <View style={styles.buttonContainer}>
          <Button
            mode="contained"
            icon="camera"
            onPress={pickImage}
            style={styles.button}
            disabled={uploading}
          >
            Take Photo
          </Button>
          <Button
            mode="outlined"
            icon="file-document"
            onPress={pickDocument}
            style={styles.button}
            disabled={uploading}
          >
            Upload PDF/MusicXML
          </Button>
        </View>
      </Card.Content>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    marginBottom: 16,
  },
  title: {
    marginBottom: 8,
  },
  subtitle: {
    marginBottom: 16,
    color: '#666',
  },
  progressContainer: {
    marginVertical: 16,
  },
  loader: {
    marginTop: 8,
  },
  successContainer: {
    backgroundColor: '#e8f5e9',
    padding: 12,
    borderRadius: 4,
    marginBottom: 16,
  },
  successText: {
    color: '#2e7d32',
  },
  buttonContainer: {
    gap: 12,
  },
  button: {
    marginBottom: 8,
  },
});

