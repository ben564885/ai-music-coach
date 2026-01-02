import React, { useState, useEffect } from 'react';
import { StyleSheet, View, Alert, Platform, PermissionsAndroid } from 'react-native';
import { Card, Button, Text, ActivityIndicator } from 'react-native-paper';
import { BleManager } from 'react-native-ble-plx';
import { T5AI_DEVICE_NAME, T5AI_SERVICE_UUID, T5AI_CHARACTERISTIC_UUID } from '../config';

const bleManager = new BleManager();

export default function DeviceConnection({ isConnected, onConnectionChange }) {
  const [scanning, setScanning] = useState(false);
  const [device, setDevice] = useState(null);

  useEffect(() => {
    // Request Bluetooth permissions
    requestPermissions();

    // Cleanup on unmount
    return () => {
      bleManager.destroy();
    };
  }, []);

  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      try {
        const granted = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        ]);
        
        if (
          granted[PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN] !== PermissionsAndroid.RESULTS.GRANTED ||
          granted[PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT] !== PermissionsAndroid.RESULTS.GRANTED
        ) {
          Alert.alert('Permissions Required', 'Bluetooth permissions are required to connect to the T5AI device.');
        }
      } catch (err) {
        console.warn('Permission request error:', err);
      }
    }
  };

  const scanForDevices = async () => {
    setScanning(true);
    
    try {
      // Check if Bluetooth is enabled
      const state = await bleManager.state();
      if (state !== 'PoweredOn') {
        Alert.alert('Bluetooth Off', 'Please enable Bluetooth to connect to the device.');
        setScanning(false);
        return;
      }

      // Start scanning
      bleManager.startDeviceScan(null, null, (error, scannedDevice) => {
        if (error) {
          console.error('Scan error:', error);
          setScanning(false);
          return;
        }

        if (scannedDevice && scannedDevice.name === T5AI_DEVICE_NAME) {
          bleManager.stopDeviceScan();
          setDevice(scannedDevice);
          connectToDevice(scannedDevice);
        }
      });

      // Stop scanning after 10 seconds
      setTimeout(() => {
        bleManager.stopDeviceScan();
        setScanning(false);
        if (!device) {
          Alert.alert('Device Not Found', `Could not find ${T5AI_DEVICE_NAME}. Make sure the device is powered on and nearby.`);
        }
      }, 10000);
    } catch (error) {
      console.error('Scan error:', error);
      Alert.alert('Error', 'Failed to scan for devices: ' + error.message);
      setScanning(false);
    }
  };

  const connectToDevice = async (deviceToConnect) => {
    try {
      const connectedDevice = await deviceToConnect.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();
      
      setDevice(connectedDevice);
      onConnectionChange(true);
      
      Alert.alert('Connected', `Successfully connected to ${T5AI_DEVICE_NAME}`);
    } catch (error) {
      console.error('Connection error:', error);
      Alert.alert('Connection Failed', 'Could not connect to device: ' + error.message);
      onConnectionChange(false);
    }
  };

  const disconnectDevice = async () => {
    try {
      if (device) {
        await device.cancelConnection();
        setDevice(null);
        onConnectionChange(false);
        Alert.alert('Disconnected', 'Device disconnected successfully');
      }
    } catch (error) {
      console.error('Disconnect error:', error);
      Alert.alert('Error', 'Failed to disconnect: ' + error.message);
    }
  };

  const sendSheetMusicData = async (musicData) => {
    if (!device || !isConnected) {
      Alert.alert('Not Connected', 'Please connect to the device first');
      return;
    }

    try {
      const service = await device.discoverServices();
      const targetService = service.find(s => s.uuid === T5AI_SERVICE_UUID);
      
      if (!targetService) {
        Alert.alert('Error', 'Service not found on device');
        return;
      }

      const characteristics = await targetService.discoverCharacteristics();
      const targetCharacteristic = characteristics.find(c => c.uuid === T5AI_CHARACTERISTIC_UUID);
      
      if (!targetCharacteristic) {
        Alert.alert('Error', 'Characteristic not found on device');
        return;
      }

      // Send music data as JSON string
      const dataString = JSON.stringify(musicData);
      await targetCharacteristic.writeWithResponse(dataString);
      
      Alert.alert('Success', 'Sheet music data sent to device');
    } catch (error) {
      console.error('Send error:', error);
      Alert.alert('Error', 'Failed to send data: ' + error.message);
    }
  };

  return (
    <Card style={styles.card}>
      <Card.Content>
        <Text variant="titleMedium" style={styles.title}>
          T5AI Device Connection
        </Text>
        
        <View style={styles.statusContainer}>
          <View style={[styles.statusIndicator, { backgroundColor: isConnected ? '#4caf50' : '#f44336' }]} />
          <Text variant="bodyMedium" style={styles.statusText}>
            {isConnected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>

        {scanning && (
          <View style={styles.scanningContainer}>
            <ActivityIndicator size="small" />
            <Text variant="bodySmall" style={styles.scanningText}>
              Scanning for devices...
            </Text>
          </View>
        )}

        <View style={styles.buttonContainer}>
          {!isConnected ? (
            <Button
              mode="contained"
              icon="bluetooth"
              onPress={scanForDevices}
              disabled={scanning}
              style={styles.button}
            >
              {scanning ? 'Scanning...' : 'Connect Device'}
            </Button>
          ) : (
            <Button
              mode="outlined"
              icon="bluetooth-off"
              onPress={disconnectDevice}
              style={styles.button}
            >
              Disconnect
            </Button>
          )}
        </View>

        {device && (
          <Text variant="bodySmall" style={styles.deviceInfo}>
            Device: {device.name || 'Unknown'}
          </Text>
        )}
      </Card.Content>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    marginBottom: 16,
  },
  title: {
    marginBottom: 12,
  },
  statusContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
  },
  statusIndicator: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginRight: 8,
  },
  statusText: {
    fontWeight: '500',
  },
  scanningContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
  },
  scanningText: {
    marginLeft: 8,
    color: '#666',
  },
  buttonContainer: {
    marginBottom: 8,
  },
  button: {
    marginBottom: 8,
  },
  deviceInfo: {
    color: '#666',
    marginTop: 8,
  },
});

