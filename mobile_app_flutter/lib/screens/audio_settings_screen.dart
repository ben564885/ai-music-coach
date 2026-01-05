import 'package:flutter/material.dart';
import '../utils/app_theme.dart';
import '../widgets/gradient_card.dart';

class AudioSettingsScreen extends StatefulWidget {
  const AudioSettingsScreen({super.key});

  @override
  State<AudioSettingsScreen> createState() => _AudioSettingsScreenState();
}

class _AudioSettingsScreenState extends State<AudioSettingsScreen> {
  double _feedbackVolume = 0.8;
  double _metronomeVolume = 0.5;
  bool _voiceGuidance = true;
  bool _noiseCancellation = true;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              AppTheme.backgroundColor,
              Color(0xFF0F0C29),
            ],
          ),
        ),
        child: CustomScrollView(
          slivers: [
            SliverAppBar(
              expandedHeight: 120,
              pinned: true,
              backgroundColor: Colors.transparent,
              flexibleSpace: const FlexibleSpaceBar(
                title: Text(
                  'Audio Settings',
                  style: TextStyle(
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
                centerTitle: false,
                titlePadding: EdgeInsets.only(left: 56, bottom: 16),
              ),
              leading: IconButton(
                icon: const Icon(Icons.arrow_back_ios_new, color: Colors.white),
                onPressed: () => Navigator.pop(context),
              ),
            ),
            SliverPadding(
              padding: const EdgeInsets.all(AppTheme.spacingM),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  _buildVolumeSection('Feedback Volume', _feedbackVolume, (val) {
                    setState(() => _feedbackVolume = val);
                  }),
                  const SizedBox(height: AppTheme.spacingL),
                  _buildVolumeSection('Metronome Volume', _metronomeVolume, (val) {
                    setState(() => _metronomeVolume = val);
                  }),
                  const SizedBox(height: AppTheme.spacingL),
                  _buildToggleSection('Voice-Guided Feedback', 'Hear real-time tips during practice', _voiceGuidance, (val) {
                    setState(() => _voiceGuidance = val);
                  }),
                  const SizedBox(height: AppTheme.spacingM),
                  _buildToggleSection('Noise Cancellation', 'Improve pitch detection in noisy rooms', _noiseCancellation, (val) {
                    setState(() => _noiseCancellation = val);
                  }),
                  const SizedBox(height: AppTheme.spacingXL),
                  _buildCalibrationCard(),
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildVolumeSection(String label, double value, ValueChanged<double> onChanged) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          label,
          style: const TextStyle(
            color: AppTheme.textSecondary,
            fontSize: 14,
            fontWeight: FontWeight.bold,
          ),
        ),
        const SizedBox(height: 8),
        GradientCard(
          child: Row(
            children: [
              const Icon(Icons.volume_mute, color: AppTheme.textSecondary),
              Expanded(
                child: Slider(
                  value: value,
                  onChanged: onChanged,
                  activeColor: AppTheme.primaryLight,
                  inactiveColor: AppTheme.textSecondary.withOpacity(0.2),
                ),
              ),
              const Icon(Icons.volume_up, color: AppTheme.textSecondary),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildToggleSection(String title, String subtitle, bool value, ValueChanged<bool> onChanged) {
    return GradientCard(
      padding: EdgeInsets.zero,
      child: SwitchListTile(
        title: Text(title, style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
        subtitle: Text(subtitle, style: const TextStyle(color: AppTheme.textSecondary)),
        value: value,
        onChanged: onChanged,
        activeColor: AppTheme.primaryLight,
      ),
    );
  }

  Widget _buildCalibrationCard() {
    return GradientCard(
      child: Column(
        children: [
          const Icon(Icons.mic, size: 40, color: AppTheme.accentCyan),
          const SizedBox(height: AppTheme.spacingM),
          const Text(
            'Calibrate Microphone',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 8),
          const Text(
            'Improve accuracy by calibrating to your room ambiance.',
            textAlign: TextAlign.center,
            style: TextStyle(color: AppTheme.textSecondary),
          ),
          const SizedBox(height: AppTheme.spacingL),
          ElevatedButton(
            onPressed: () {},
            style: ElevatedButton.styleFrom(
              backgroundColor: AppTheme.primaryLight,
              foregroundColor: Colors.white,
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(12),
              ),
            ),
            child: const Padding(
              padding: EdgeInsets.symmetric(horizontal: 24, vertical: 12),
              child: Text('Start Calibration'),
            ),
          ),
        ],
      ),
    );
  }
}
