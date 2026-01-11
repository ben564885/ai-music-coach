import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:http/http.dart' as http;
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:flutter_dotenv/flutter_dotenv.dart';
import '../repositories/recordings_repository.dart';
import '../repositories/sheet_music_repository.dart';
import '../models/sheet_music.dart';
import '../utils/app_theme.dart';

class AnalyzePerformanceScreen extends StatefulWidget {
  const AnalyzePerformanceScreen({super.key});

  @override
  State<AnalyzePerformanceScreen> createState() => _AnalyzePerformanceScreenState();
}

class _AnalyzePerformanceScreenState extends State<AnalyzePerformanceScreen> {
  List<Recording> _recordings = [];
  List<SheetMusic> _uploads = [];
  Recording? _selectedRecording;
  SheetMusic? _selectedUpload;
  bool _isLoading = true;
  bool _isAnalyzing = false;
  String? _feedbackResult;
  String? _errorMessage;

  @override
  void initState() {
    super.initState();
    _loadData();
  }

  Future<void> _loadData() async {
    setState(() {
      _isLoading = true;
      _errorMessage = null;
    });

    try {
      final recordingsRepo = context.read<RecordingsRepository>();
      final sheetMusicRepo = context.read<SheetMusicRepository>();

      final recordings = await recordingsRepo.getRecordings();
      final uploads = await sheetMusicRepo.getSheetMusic();

      setState(() {
        _recordings = recordings;
        _uploads = uploads;
        _isLoading = false;
      });
    } catch (e) {
      setState(() {
        _errorMessage = 'Failed to load data: $e';
        _isLoading = false;
      });
    }
  }

  Future<void> _analyzePerformance() async {
    if (_selectedRecording == null || _selectedUpload == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Please select both a recording and sheet music'),
          backgroundColor: AppTheme.errorColor,
        ),
      );
      return;
    }

    setState(() {
      _isAnalyzing = true;
      _feedbackResult = null;
      _errorMessage = null;
    });

    try {
      // Get the backend URL
      final backendUrl = dotenv.env['BACKEND_URL'] ?? 'http://192.168.34.176:5001';
      
      // Get auth token
      final token = Supabase.instance.client.auth.currentSession?.accessToken;
      
      // Call the analysis endpoint
      final response = await http.post(
        Uri.parse('$backendUrl/api/analyze-with-audio'),
        headers: {
          'Content-Type': 'application/json',
          if (token != null) 'Authorization': 'Bearer $token',
        },
        body: jsonEncode({
          'recording_id': _selectedRecording!.id,
          'audio_url': _selectedRecording!.audioUrl,
          'sheet_music_id': _selectedUpload!.id,
          'audiveris_raw_output': _selectedUpload!.audiverisRawOutput ?? '',
          'sheet_music_title': _selectedUpload!.title,
        }),
      );

      if (response.statusCode == 200) {
        final data = jsonDecode(response.body);
        setState(() {
          _feedbackResult = data['feedback'] ?? data['text'] ?? 'Analysis complete!';
          _isAnalyzing = false;
        });
      } else {
        throw Exception('Analysis failed: ${response.body}');
      }
    } catch (e) {
      setState(() {
        _errorMessage = 'Analysis failed: $e';
        _isAnalyzing = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [
              const Color(0xFF1A1A2E),
              const Color(0xFF16213E),
              const Color(0xFF0F3460),
            ],
          ),
        ),
        child: SafeArea(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // App Bar
              Padding(
                padding: const EdgeInsets.all(16),
                child: Row(
                  children: [
                    IconButton(
                      onPressed: () => Navigator.pop(context),
                      icon: Container(
                        padding: const EdgeInsets.all(8),
                        decoration: BoxDecoration(
                          color: Colors.white.withOpacity(0.1),
                          borderRadius: BorderRadius.circular(10),
                        ),
                        child: const Icon(
                          Icons.arrow_back_ios_new,
                          color: Colors.white,
                          size: 20,
                        ),
                      ),
                    ),
                    const SizedBox(width: 12),
                    const Expanded(
                      child: Text(
                        'Analyze Performance',
                        style: TextStyle(
                          fontSize: 24,
                          fontWeight: FontWeight.bold,
                          color: Colors.white,
                        ),
                      ),
                    ),
                  ],
                ),
              ),

              // Content
              Expanded(
                child: _isLoading
                    ? const Center(
                        child: CircularProgressIndicator(color: Colors.white),
                      )
                    : _errorMessage != null && _feedbackResult == null
                        ? _buildErrorState()
                        : _feedbackResult != null
                            ? _buildFeedbackResult()
                            : _buildSelectionUI(),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildErrorState() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              Icons.error_outline,
              size: 64,
              color: AppTheme.errorColor.withOpacity(0.7),
            ),
            const SizedBox(height: 16),
            Text(
              _errorMessage!,
              textAlign: TextAlign.center,
              style: TextStyle(
                color: Colors.white.withOpacity(0.7),
                fontSize: 16,
              ),
            ),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              onPressed: _loadData,
              icon: const Icon(Icons.refresh),
              label: const Text('Try Again'),
              style: ElevatedButton.styleFrom(
                backgroundColor: AppTheme.primaryColor,
                foregroundColor: Colors.white,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSelectionUI() {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Recording Selection
          _buildSectionHeader(
            'Select Recording',
            Icons.mic_rounded,
            const Color(0xFF667EEA),
          ),
          const SizedBox(height: 12),
          _buildRecordingSelector(),

          const SizedBox(height: 32),

          // Upload Selection
          _buildSectionHeader(
            'Select Sheet Music',
            Icons.music_note_rounded,
            const Color(0xFF11998E),
          ),
          const SizedBox(height: 12),
          _buildUploadSelector(),

          const SizedBox(height: 40),

          // Analyze Button
          _buildAnalyzeButton(),
        ],
      ),
    );
  }

  Widget _buildSectionHeader(String title, IconData icon, Color color) {
    return Row(
      children: [
        Container(
          padding: const EdgeInsets.all(8),
          decoration: BoxDecoration(
            color: color.withOpacity(0.2),
            borderRadius: BorderRadius.circular(10),
          ),
          child: Icon(icon, color: color, size: 20),
        ),
        const SizedBox(width: 12),
        Text(
          title,
          style: const TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.bold,
            color: Colors.white,
          ),
        ),
      ],
    );
  }

  Widget _buildRecordingSelector() {
    if (_recordings.isEmpty) {
      return _buildEmptyCard(
        'No recordings yet',
        'Record a practice session to analyze',
        Icons.mic_off,
      );
    }

    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: Colors.white.withOpacity(0.1),
        ),
      ),
      child: ListView.separated(
        shrinkWrap: true,
        physics: const NeverScrollableScrollPhysics(),
        itemCount: _recordings.length,
        separatorBuilder: (_, __) => Divider(
          color: Colors.white.withOpacity(0.1),
          height: 1,
        ),
        itemBuilder: (context, index) {
          final recording = _recordings[index];
          final isSelected = _selectedRecording?.id == recording.id;
          
          return _SelectableListTile(
            title: recording.title,
            subtitle: _formatDate(recording.createdAt),
            isSelected: isSelected,
            onTap: () {
              setState(() {
                _selectedRecording = isSelected ? null : recording;
              });
            },
            leadingIcon: Icons.audiotrack_rounded,
            selectedColor: const Color(0xFF667EEA),
          );
        },
      ),
    );
  }

  Widget _buildUploadSelector() {
    if (_uploads.isEmpty) {
      return _buildEmptyCard(
        'No sheet music yet',
        'Upload sheet music to compare against',
        Icons.music_off,
      );
    }

    return Container(
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: Colors.white.withOpacity(0.1),
        ),
      ),
      child: ListView.separated(
        shrinkWrap: true,
        physics: const NeverScrollableScrollPhysics(),
        itemCount: _uploads.length,
        separatorBuilder: (_, __) => Divider(
          color: Colors.white.withOpacity(0.1),
          height: 1,
        ),
        itemBuilder: (context, index) {
          final upload = _uploads[index];
          final isSelected = _selectedUpload?.id == upload.id;
          final hasAudiveris = upload.audiverisRawOutput != null && 
                               upload.audiverisRawOutput!.isNotEmpty;
          
          return _SelectableListTile(
            title: upload.title,
            subtitle: '${upload.referenceData['timeSignature'] ?? '4/4'} • ${upload.friendlyKeySignature}${hasAudiveris ? ' • ✓ Analyzed' : ''}',
            isSelected: isSelected,
            onTap: () {
              setState(() {
                _selectedUpload = isSelected ? null : upload;
              });
            },
            leadingIcon: Icons.music_note,
            selectedColor: const Color(0xFF11998E),
          );
        },
      ),
    );
  }

  Widget _buildEmptyCard(String title, String subtitle, IconData icon) {
    return Container(
      padding: const EdgeInsets.all(32),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.05),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: Colors.white.withOpacity(0.1),
        ),
      ),
      child: Column(
        children: [
          Icon(
            icon,
            size: 48,
            color: Colors.white.withOpacity(0.3),
          ),
          const SizedBox(height: 16),
          Text(
            title,
            style: TextStyle(
              fontSize: 16,
              fontWeight: FontWeight.bold,
              color: Colors.white.withOpacity(0.7),
            ),
          ),
          const SizedBox(height: 4),
          Text(
            subtitle,
            style: TextStyle(
              fontSize: 14,
              color: Colors.white.withOpacity(0.5),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildAnalyzeButton() {
    final canAnalyze = _selectedRecording != null && _selectedUpload != null;

    return GestureDetector(
      onTap: canAnalyze && !_isAnalyzing ? _analyzePerformance : null,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        width: double.infinity,
        padding: const EdgeInsets.symmetric(vertical: 18),
        decoration: BoxDecoration(
          gradient: canAnalyze
              ? const LinearGradient(
                  colors: [Color(0xFF667EEA), Color(0xFF764BA2)],
                )
              : null,
          color: canAnalyze ? null : Colors.white.withOpacity(0.1),
          borderRadius: BorderRadius.circular(16),
          boxShadow: canAnalyze
              ? [
                  BoxShadow(
                    color: const Color(0xFF667EEA).withOpacity(0.4),
                    blurRadius: 20,
                    offset: const Offset(0, 10),
                  ),
                ]
              : null,
        ),
        child: _isAnalyzing
            ? const Center(
                child: SizedBox(
                  height: 24,
                  width: 24,
                  child: CircularProgressIndicator(
                    strokeWidth: 2,
                    color: Colors.white,
                  ),
                ),
              )
            : Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(
                    Icons.auto_awesome,
                    color: canAnalyze
                        ? Colors.white
                        : Colors.white.withOpacity(0.3),
                  ),
                  const SizedBox(width: 12),
                  Text(
                    'Analyze with AI',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                      color: canAnalyze
                          ? Colors.white
                          : Colors.white.withOpacity(0.3),
                    ),
                  ),
                ],
              ),
      ),
    );
  }

  Widget _buildFeedbackResult() {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Success Header
          Container(
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              gradient: const LinearGradient(
                colors: [Color(0xFF11998E), Color(0xFF38EF7D)],
              ),
              borderRadius: BorderRadius.circular(16),
            ),
            child: Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: const Icon(
                    Icons.check_circle,
                    color: Colors.white,
                    size: 28,
                  ),
                ),
                const SizedBox(width: 16),
                const Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Analysis Complete',
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: Colors.white,
                        ),
                      ),
                      SizedBox(height: 4),
                      Text(
                        'Here\'s your AI-powered feedback',
                        style: TextStyle(
                          fontSize: 14,
                          color: Colors.white70,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 24),

          // Feedback Content
          Container(
            width: double.infinity,
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.05),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(
                color: Colors.white.withOpacity(0.1),
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Icon(
                      Icons.auto_awesome,
                      color: Colors.amber.withOpacity(0.8),
                      size: 20,
                    ),
                    const SizedBox(width: 8),
                    Text(
                      'AI Feedback',
                      style: TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.bold,
                        color: Colors.white.withOpacity(0.6),
                        letterSpacing: 1,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 16),
                Text(
                  _feedbackResult!,
                  style: const TextStyle(
                    fontSize: 16,
                    color: Colors.white,
                    height: 1.6,
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 24),

          // New Analysis Button
          GestureDetector(
            onTap: () {
              setState(() {
                _feedbackResult = null;
                _selectedRecording = null;
                _selectedUpload = null;
              });
            },
            child: Container(
              width: double.infinity,
              padding: const EdgeInsets.symmetric(vertical: 16),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.1),
                borderRadius: BorderRadius.circular(12),
                border: Border.all(
                  color: Colors.white.withOpacity(0.2),
                ),
              ),
              child: const Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(Icons.replay, color: Colors.white70),
                  SizedBox(width: 8),
                  Text(
                    'Analyze Another',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w500,
                      color: Colors.white70,
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

  String _formatDate(DateTime date) {
    final now = DateTime.now();
    final diff = now.difference(date);

    if (diff.inDays == 0) {
      return 'Today at ${date.hour.toString().padLeft(2, '0')}:${date.minute.toString().padLeft(2, '0')}';
    } else if (diff.inDays == 1) {
      return 'Yesterday';
    } else if (diff.inDays < 7) {
      return '${diff.inDays} days ago';
    } else {
      return '${date.month}/${date.day}/${date.year}';
    }
  }
}

class _SelectableListTile extends StatelessWidget {
  final String title;
  final String subtitle;
  final bool isSelected;
  final VoidCallback onTap;
  final IconData leadingIcon;
  final Color selectedColor;

  const _SelectableListTile({
    required this.title,
    required this.subtitle,
    required this.isSelected,
    required this.onTap,
    required this.leadingIcon,
    required this.selectedColor,
  });

  @override
  Widget build(BuildContext context) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(12),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
        decoration: BoxDecoration(
          color: isSelected 
              ? selectedColor.withOpacity(0.15) 
              : Colors.transparent,
          borderRadius: BorderRadius.circular(12),
        ),
        child: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(10),
              decoration: BoxDecoration(
                color: isSelected
                    ? selectedColor.withOpacity(0.3)
                    : Colors.white.withOpacity(0.1),
                borderRadius: BorderRadius.circular(10),
              ),
              child: Icon(
                leadingIcon,
                color: isSelected ? selectedColor : Colors.white54,
                size: 20,
              ),
            ),
            const SizedBox(width: 14),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    title,
                    style: TextStyle(
                      fontSize: 15,
                      fontWeight: FontWeight.w600,
                      color: isSelected ? Colors.white : Colors.white.withOpacity(0.9),
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  const SizedBox(height: 2),
                  Text(
                    subtitle,
                    style: TextStyle(
                      fontSize: 13,
                      color: Colors.white.withOpacity(0.5),
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ],
              ),
            ),
            AnimatedContainer(
              duration: const Duration(milliseconds: 200),
              width: 24,
              height: 24,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: isSelected ? selectedColor : Colors.transparent,
                border: Border.all(
                  color: isSelected ? selectedColor : Colors.white.withOpacity(0.3),
                  width: 2,
                ),
              ),
              child: isSelected
                  ? const Icon(
                      Icons.check,
                      size: 14,
                      color: Colors.white,
                    )
                  : null,
            ),
          ],
        ),
      ),
    );
  }
}

