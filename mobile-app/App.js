import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { Provider as PaperProvider, ActivityIndicator, View, Button } from 'react-native-paper';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { supabase } from './src/services/supabase';
import AuthScreen from './src/screens/AuthScreen';
import RecordingsScreen from './src/screens/RecordingsScreen';
import RecordingDetailScreen from './src/screens/RecordingDetailScreen';
import NewRecordingScreen from './src/screens/NewRecordingScreen';

const Stack = createNativeStackNavigator();

export default function App() {
  const [session, setSession] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Check active session
    supabase.auth.getSession().then(({ data: { session } }) => {
      setSession(session);
      setLoading(false);
    });

    // Listen for auth changes
    const {
      data: { subscription },
    } = supabase.auth.onAuthStateChange((_event, session) => {
      setSession(session);
    });

    return () => subscription.unsubscribe();
  }, []);

  if (loading) {
    return (
      <PaperProvider>
        <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
          <ActivityIndicator size="large" />
        </View>
      </PaperProvider>
    );
  }

  return (
    <PaperProvider>
      <SafeAreaProvider>
        <NavigationContainer>
          <Stack.Navigator
            screenOptions={{
              headerStyle: {
                backgroundColor: '#6200ee',
              },
              headerTintColor: '#fff',
              headerTitleStyle: {
                fontWeight: 'bold',
              },
            }}
          >
            {!session ? (
              <Stack.Screen
                name="Auth"
                component={AuthScreen}
                options={{ headerShown: false }}
              />
            ) : (
              <>
                <Stack.Screen
                  name="Recordings"
                  component={RecordingsScreen}
                  options={{
                    title: 'My Recordings',
                    headerRight: () => (
                      <Button
                        mode="text"
                        onPress={() => supabase.auth.signOut()}
                        textColor="#fff"
                        style={{ marginRight: 8 }}
                      >
                        Sign Out
                      </Button>
                    ),
                  }}
                />
                <Stack.Screen
                  name="RecordingDetail"
                  component={RecordingDetailScreen}
                  options={{ title: 'Recording Details' }}
                />
                <Stack.Screen
                  name="NewRecording"
                  component={NewRecordingScreen}
                  options={{ title: 'New Recording' }}
                />
              </>
            )}
          </Stack.Navigator>
        </NavigationContainer>
      </SafeAreaProvider>
    </PaperProvider>
  );
}
