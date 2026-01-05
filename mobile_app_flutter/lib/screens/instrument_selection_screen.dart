import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:mobile_app_flutter/blocs/auth/auth_bloc.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/screens/home_screen.dart';

class Instrument {
  final String name;
  final IconData icon;
  final String emoji; // Emoji icon for better visual representation
  final int value; // Matches firmware InstrumentType enum

  const Instrument({
    required this.name,
    required this.icon,
    required this.emoji,
    required this.value,
  });
}

class InstrumentSelectionScreen extends StatefulWidget {
  final bool isFromSettings;
  
  const InstrumentSelectionScreen({
    super.key,
    this.isFromSettings = false,
  });

  @override
  State<InstrumentSelectionScreen> createState() => _InstrumentSelectionScreenState();
}

class _InstrumentSelectionScreenState extends State<InstrumentSelectionScreen> {
  final _supabase = Supabase.instance.client;
  int? _selectedInstrument;
  bool _isLoading = false;

  @override
  void initState() {
    super.initState();
    // Load current instrument if coming from settings
    if (widget.isFromSettings) {
      _loadCurrentInstrument();
    }
  }

  void _loadCurrentInstrument() {
    final user = _supabase.auth.currentUser;
    if (user != null) {
      final metadata = user.userMetadata ?? {};
      final prefs = metadata['preferences'] as Map<String, dynamic>? ?? {};
      final instrumentValue = prefs['instrument_value'];
      if (instrumentValue != null) {
        setState(() {
          _selectedInstrument = instrumentValue as int;
        });
      }
    }
  }

  static const List<Instrument> instruments = [
    Instrument(name: 'Piano', icon: Icons.piano, emoji: '🎹', value: 0),
    Instrument(name: 'Violin', icon: Icons.music_note, emoji: '🎻', value: 1),
    Instrument(name: 'Guitar', icon: Icons.music_note, emoji: '🎸', value: 2),
    Instrument(name: 'Flute', icon: Icons.music_note, emoji: '🎵', value: 3),
    Instrument(name: 'Clarinet', icon: Icons.music_note, emoji: '🎼', value: 4), // Using musical score as closest alternative
    Instrument(name: 'Trumpet', icon: Icons.music_note, emoji: '🎺', value: 5),
    Instrument(name: 'Saxophone', icon: Icons.music_note, emoji: '🎷', value: 6),
  ];

  Future<void> _saveInstrument() async {
    if (_selectedInstrument == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Please select an instrument'),
          backgroundColor: Colors.orange,
        ),
      );
      return;
    }

    setState(() => _isLoading = true);

    try {
      final user = _supabase.auth.currentUser;
      if (user == null) return;

      final currentMetadata = user.userMetadata ?? {};
      final currentPrefs = Map<String, dynamic>.from(
        currentMetadata['preferences'] as Map? ?? {},
      );

      // Find selected instrument name
      final selectedInstrumentName = instruments
          .firstWhere((inst) => inst.value == _selectedInstrument)
          .name;

      currentPrefs['instrument'] = selectedInstrumentName;
      currentPrefs['instrument_value'] = _selectedInstrument;

      await _supabase.auth.updateUser(
        UserAttributes(
          data: {
            ...currentMetadata,
            'preferences': currentPrefs,
          },
        ),
      );

      if (mounted) {
        context.read<AuthBloc>().add(const AuthUserMetadataUpdated());
        // Navigate based on context
        await Future.delayed(const Duration(milliseconds: 300));
        if (mounted) {
          if (widget.isFromSettings) {
            // If from settings, just go back
            Navigator.of(context).pop();
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(
                content: Text('Instrument updated successfully'),
                backgroundColor: Colors.green,
              ),
            );
          } else {
            // If initial setup, go to home screen
            Navigator.of(context).pushReplacement(
              MaterialPageRoute(builder: (context) => const HomeScreen()),
            );
          }
        }
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Error saving instrument: $e'),
            backgroundColor: Colors.red,
          ),
        );
      }
    } finally {
      if (mounted) {
        setState(() => _isLoading = false);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: widget.isFromSettings
            ? const BoxDecoration(
                gradient: AppTheme.backgroundGradient,
              )
            : const BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                  colors: [
                    Color(0xFF6C63FF), // Primary Violet
                    Color(0xFF000000), // Black
                  ],
                ),
              ),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.all(24.0),
            child: Column(
              mainAxisAlignment: widget.isFromSettings ? MainAxisAlignment.start : MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                if (!widget.isFromSettings) ...[
                  const Icon(
                    Icons.music_note_rounded,
                    size: 80,
                    color: Colors.white,
                  ),
                  const SizedBox(height: 24),
                  Text(
                    'What instrument do you want to learn?',
                    textAlign: TextAlign.center,
                    style: Theme.of(context).textTheme.displaySmall?.copyWith(
                          color: Colors.white,
                          fontWeight: FontWeight.bold,
                        ),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Select your primary instrument to get personalized feedback',
                    textAlign: TextAlign.center,
                    style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                          color: Colors.white70,
                        ),
                  ),
                ] else ...[
                  // Back button
                  Row(
                    children: [
                      IconButton(
                        icon: const Icon(Icons.arrow_back, color: Colors.white),
                        onPressed: () => Navigator.of(context).pop(),
                        padding: EdgeInsets.zero,
                        constraints: const BoxConstraints(),
                      ),
                    ],
                  ),
                  const SizedBox(height: 16),
                  Text(
                    'Select your instrument',
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                          fontWeight: FontWeight.bold,
                        ),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Choose your instrument to get personalized feedback',
                    style: Theme.of(context).textTheme.bodyMedium,
                  ),
                ],
                const SizedBox(height: 32),
                Expanded(
                  child: GridView.builder(
                    gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                      crossAxisCount: 2,
                      crossAxisSpacing: 16,
                      mainAxisSpacing: 16,
                      childAspectRatio: 1.1,
                    ),
                    itemCount: instruments.length,
                    shrinkWrap: false,
                    itemBuilder: (context, index) {
                      final instrument = instruments[index];
                      final isSelected = _selectedInstrument == instrument.value;

                      return GestureDetector(
                        onTap: () {
                          setState(() {
                            _selectedInstrument = instrument.value;
                          });
                        },
                        child: AnimatedContainer(
                          duration: const Duration(milliseconds: 200),
                          decoration: BoxDecoration(
                            color: isSelected
                                ? Colors.white.withOpacity(0.2)
                                : Colors.white.withOpacity(0.1),
                            borderRadius: BorderRadius.circular(20),
                            border: Border.all(
                              color: isSelected
                                  ? Colors.white
                                  : Colors.white.withOpacity(0.3),
                              width: isSelected ? 3 : 1,
                            ),
                            boxShadow: isSelected
                                ? [
                                    BoxShadow(
                                      color: Colors.white.withOpacity(0.3),
                                      blurRadius: 20,
                                      spreadRadius: 2,
                                    ),
                                  ]
                                : null,
                          ),
                          child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              Text(
                                instrument.emoji,
                                style: const TextStyle(fontSize: 48),
                              ),
                              const SizedBox(height: 12),
                              Text(
                                instrument.name,
                                style: const TextStyle(
                                  color: Colors.white,
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              if (isSelected) ...[
                                const SizedBox(height: 8),
                                const Icon(
                                  Icons.check_circle,
                                  color: Colors.white,
                                  size: 24,
                                ),
                              ],
                            ],
                          ),
                        ),
                      );
                    },
                  ),
                ),
                const SizedBox(height: 24),
                if (_isLoading)
                  const Center(
                    child: CircularProgressIndicator(color: Colors.white),
                  )
                else
                  ElevatedButton(
                    onPressed: _saveInstrument,
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.symmetric(vertical: 16),
                      backgroundColor: widget.isFromSettings ? AppTheme.primaryColor : Colors.white,
                      foregroundColor: widget.isFromSettings ? Colors.white : const Color(0xFF6C63FF),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(16),
                      ),
                    ),
                    child: Text(
                      widget.isFromSettings ? 'Save' : 'Continue',
                      style: const TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ),
                if (widget.isFromSettings) const SizedBox(height: 16),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

