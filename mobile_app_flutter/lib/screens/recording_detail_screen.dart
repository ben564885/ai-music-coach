
import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:audioplayers/audioplayers.dart';
import '../repositories/recordings_repository.dart';
import '../services/audio_service.dart';

class RecordingDetailScreen extends StatefulWidget {
  final Recording recording;

  const RecordingDetailScreen({super.key, required this.recording});

  @override
  State<RecordingDetailScreen> createState() => _RecordingDetailScreenState();
}

class _RecordingDetailScreenState extends State<RecordingDetailScreen> {
  late final AudioService _audioService;
  PlayerState _playerState = PlayerState.stopped;
  Duration _position = Duration.zero;
  Duration _duration = Duration.zero;

  @override
  void initState() {
    super.initState();
    _audioService = context.read<AudioService>();
    
    _audioService.playerState.listen((state) {
      if (mounted) setState(() => _playerState = state);
    });
    _audioService.position.listen((pos) {
      if (mounted) setState(() => _position = pos);
    });
    _audioService.duration.listen((dur) {
      if (mounted) setState(() => _duration = dur);
    });
  }

  @override
  void dispose() {
    // We don't dispose audioService here because it is provided from above and might be shared?
    // Actually in main.dart I should create it.
    // Ideally stop playback on exit
    _audioService.stop();
    super.dispose();
  }

  void _playPause() {
    if (_playerState == PlayerState.playing) {
      _audioService.pause();
    } else {
      if (widget.recording.audioUrl != null) {
        _audioService.play(widget.recording.audioUrl!);
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('No audio url available')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Performance Review')),
      body: Column(
        children: [
          // Visualizer Area
          Expanded(
            flex: 4,
            child: Container(
              width: double.infinity,
              padding: const EdgeInsets.all(24),
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  begin: Alignment.topCenter,
                  end: Alignment.bottomCenter,
                  colors: [
                    Theme.of(context).primaryColor.withValues(alpha: 0.1),
                    Theme.of(context).scaffoldBackgroundColor,
                  ],
                ),
              ),
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Text(
                    widget.recording.title,
                    style: Theme.of(context).textTheme.displayMedium,
                    textAlign: TextAlign.center,
                  ),
                  const SizedBox(height: 32),
                  // Fake Visualizer Bars
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    crossAxisAlignment: CrossAxisAlignment.end,
                    children: List.generate(10, (index) {
                      return Container(
                        width: 8,
                        height: 30.0 + (index % 3 * 20), // Random-ish heights
                        margin: const EdgeInsets.symmetric(horizontal: 4),
                        decoration: BoxDecoration(
                          color: _playerState == PlayerState.playing
                              ? Theme.of(context).primaryColor
                              : Colors.white24,
                          borderRadius: BorderRadius.circular(4),
                        ),
                      );
                    }),
                  ),
                ],
              ),
            ),
          ),
          
          // Controls Area
          Expanded(
             flex: 3,
             child: Container(
               decoration: BoxDecoration(
                 color: Theme.of(context).cardColor,
                 borderRadius: const BorderRadius.vertical(top: Radius.circular(32)),
               ),
               padding: const EdgeInsets.all(24),
               child: Column(
                 children: [
                   Slider(
                    value: _position.inSeconds.toDouble(),
                    max: _duration.inSeconds.toDouble() > 0 ? _duration.inSeconds.toDouble() : 1,
                    activeColor: Theme.of(context).primaryColor,
                    onChanged: (value) {
                      _audioService.seek(Duration(seconds: value.toInt()));
                    },
                  ),
                  Padding(
                    padding: const EdgeInsets.symmetric(horizontal: 16),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text(_formatDuration(_position)),
                        Text(_formatDuration(_duration)),
                      ],
                    ),
                  ),
                  const SizedBox(height: 24),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      IconButton(
                        icon: const Icon(Icons.replay_10),
                        iconSize: 32,
                        onPressed: () => _audioService.seek(_position - const Duration(seconds: 10)),
                      ),
                      const SizedBox(width: 32),
                      Container(
                        decoration: BoxDecoration(
                          color: Theme.of(context).primaryColor,
                          shape: BoxShape.circle,
                          boxShadow: [
                            BoxShadow(
                              color: Theme.of(context).primaryColor.withValues(alpha: 0.4),
                              blurRadius: 10,
                              spreadRadius: 2,
                            )
                          ]
                        ),
                        child: IconButton(
                          icon: Icon(_playerState == PlayerState.playing ? Icons.pause : Icons.play_arrow),
                          iconSize: 48,
                          color: Colors.white,
                          onPressed: _playPause,
                        ),
                      ),
                       const SizedBox(width: 32),
                      IconButton(
                        icon: const Icon(Icons.forward_10),
                        iconSize: 32,
                        onPressed: () => _audioService.seek(_position + const Duration(seconds: 10)),
                      ),
                    ],
                  ),
                 ],
               ),
             ),
          ),
          
          // Feedback Area
          Expanded(
            flex: 3,
             child: Padding(
               padding: const EdgeInsets.all(24.0),
               child: Column(
                 crossAxisAlignment: CrossAxisAlignment.start,
                 children: [
                   Text('AI Coach Feedback', style: Theme.of(context).textTheme.titleLarge),
                   const SizedBox(height: 8),
                   Expanded(
                     child: SingleChildScrollView(
                       child: Container(
                         padding: const EdgeInsets.all(16),
                         width: double.infinity,
                         decoration: BoxDecoration(
                           color: Theme.of(context).colorScheme.surface,
                           borderRadius: BorderRadius.circular(16),
                           border: Border.all(color: Colors.white10),
                         ),
                         child: Text(
                           widget.recording.feedback != null 
                             ? widget.recording.feedback.toString() 
                             : "No specific feedback generated for this session. Keep practicing!",
                           style: Theme.of(context).textTheme.bodyLarge,
                         ),
                       ),
                     ),
                   ),
                 ],
               ),
             ),
          ),
        ],
      ),
    );
  }

  String _formatDuration(Duration duration) {
    String twoDigits(int n) => n.toString().padLeft(2, '0');
    final minutes = twoDigits(duration.inMinutes.remainder(60));
    final seconds = twoDigits(duration.inSeconds.remainder(60));
    return '$minutes:$seconds';
  }
}
