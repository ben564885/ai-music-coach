import 'dart:io';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:image_picker/image_picker.dart';
import 'package:file_picker/file_picker.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/utils/app_localizations.dart';
import 'package:mobile_app_flutter/widgets/gradient_card.dart';

enum UploadSource { camera, device }

class SheetMusicProcessScreen extends StatefulWidget {
  final UploadSource initialSource;

  const SheetMusicProcessScreen({
    super.key,
    required this.initialSource,
  });

  @override
  State<SheetMusicProcessScreen> createState() => _SheetMusicProcessScreenState();
}

class _SheetMusicProcessScreenState extends State<SheetMusicProcessScreen> {
  File? _file;
  bool _isProcessing = false;
  String? _transcriptionResult;
  String? _fileUrl;
  String? _sheetMusicId;
  final TextEditingController _nameController = TextEditingController();

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _handleSourceSelection(widget.initialSource);
    });
  }

  Future<void> _handleSourceSelection(UploadSource source) async {
    if (source == UploadSource.camera) {
      await _pickFromCamera();
    } else {
      await _pickFromFile();
    }
  }

  Future<void> _pickFromCamera() async {
    final picker = ImagePicker();
    final pickedFile = await picker.pickImage(source: ImageSource.camera);
    if (pickedFile != null) {
      setState(() => _file = File(pickedFile.path));
      _startTranscription();
    } else if (_file == null) {
      Navigator.pop(context);
    }
  }

  Future<void> _pickFromFile() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['jpg', 'png', 'pdf'],
    );
    if (result != null) {
      setState(() => _file = File(result.files.single.path!));
      _startTranscription();
    } else if (_file == null) {
      Navigator.pop(context);
    }
  }

  Future<void> _startTranscription() async {
    if (_file == null) return;

    setState(() => _isProcessing = true);

    try {
      final request = http.MultipartRequest(
        'POST',
        Uri.parse('http://10.0.0.146:5001/api/upload-sheet-music'), // Using Mac local IP and port 5001
      );

      final session = Supabase.instance.client.auth.currentSession;
      if (session != null) {
        request.headers['Authorization'] = 'Bearer ${session.accessToken}';
      }

      request.files.add(await http.MultipartFile.fromPath('file', _file!.path));
      
      final streamedResponse = await request.send();
      final response = await http.Response.fromStream(streamedResponse);

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        print('Upload response: $data');
        setState(() {
          _transcriptionResult = json.encode(data['transcription']);
          _fileUrl = data['sheet_music']['file_url'];
          _sheetMusicId = data['sheet_music']['id']?.toString();
          print('Sheet music ID from backend: $_sheetMusicId');
        });
      } else {
        final errorBody = response.body;
        print('Server error ${response.statusCode}: $errorBody');
        throw Exception('Server error: ${response.statusCode} - $errorBody');
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Transcription failed: $e')),
        );
      }
    } finally {
      if (mounted) setState(() => _isProcessing = false);
    }
  }

  Future<void> _saveExcerpt() async {
    if (_transcriptionResult == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('No transcription data available')),
      );
      return;
    }
    
    // Require a title - don't allow empty
    final title = _nameController.text.trim();
    if (title.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Please enter a title for this piece'),
          backgroundColor: Colors.orange,
        ),
      );
      return;
    }

    setState(() => _isProcessing = true);

    try {
      final supabase = Supabase.instance.client;
      final userId = supabase.auth.currentUser!.id;

      if (_sheetMusicId != null) {
        // Update the record created by the backend with the user's name
        print('Updating sheet music with ID: $_sheetMusicId, title: $title');
        final updateResult = await supabase
            .from('sheet_music')
            .update({
              'title': title,
            })
            .eq('id', _sheetMusicId!)
            .select();
        
        print('Update result: $updateResult');
        
        if (updateResult.isEmpty) {
          throw Exception('Update failed: No rows updated');
        }
      } else {
        // Fallback: This shouldn't really happen with the current backend
        print('Warning: No sheet music ID, creating new record');
        await supabase.from('sheet_music').insert({
          'user_id': userId,
          'title': title,
          'reference_data': json.decode(_transcriptionResult!),
          'file_url': _fileUrl ?? '',
        });
      }

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Successfully saved "$title"'),
            backgroundColor: AppTheme.successColor,
          ),
        );
        Navigator.pop(context);
      }
    } catch (e) {
      print('Error saving excerpt: $e');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to save excerpt: $e')),
        );
      }
    } finally {
      if (mounted) setState(() => _isProcessing = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);

    return Scaffold(
      appBar: AppBar(
        title: Text(l10n.translate('upload_music')),
        backgroundColor: Colors.transparent,
        elevation: 0,
      ),
      extendBodyBehindAppBar: true,
      body: Container(
        decoration: const BoxDecoration(
          gradient: AppTheme.backgroundGradient,
        ),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.all(AppTheme.spacingL),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                if (_isProcessing) ...[
                  const CircularProgressIndicator(color: AppTheme.primaryLight),
                  const SizedBox(height: AppTheme.spacingXL),
                  Text(
                    l10n.translate('transcribing'),
                    style: AppTheme.textTheme.headlineSmall,
                  ),
                ] else if (_transcriptionResult != null) ...[
                  GradientCard(
                    padding: const EdgeInsets.all(AppTheme.spacingL),
                    child: Column(
                      children: [
                        Text(
                          l10n.translate('name_excerpt'),
                          style: AppTheme.textTheme.titleLarge,
                        ),
                        const SizedBox(height: AppTheme.spacingM),
                        TextField(
                          controller: _nameController,
                          autofocus: true,
                          style: const TextStyle(color: Colors.white, fontSize: 18),
                          decoration: InputDecoration(
                            labelText: 'Title *',
                            labelStyle: const TextStyle(color: Colors.white70),
                            hintText: 'Enter a name for this piece',
                            hintStyle: const TextStyle(color: Colors.white38),
                            enabledBorder: const UnderlineInputBorder(
                              borderSide: BorderSide(color: Colors.white24),
                            ),
                            focusedBorder: const UnderlineInputBorder(
                              borderSide: BorderSide(color: Colors.white, width: 2),
                            ),
                          ),
                        ),
                        const SizedBox(height: AppTheme.spacingS),
                        Text(
                          l10n.translate('reference_ai_hint'),
                          style: AppTheme.textTheme.bodySmall?.copyWith(
                            color: Colors.white54,
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(height: AppTheme.spacingXL),
                  ElevatedButton(
                    onPressed: _saveExcerpt,
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.symmetric(
                        horizontal: AppTheme.spacingXL,
                        vertical: AppTheme.spacingM,
                      ),
                    ),
                    child: Text(l10n.translate('save')),
                  ),
                ] else ...[
                  const Icon(Icons.music_note, size: 80, color: Colors.white24),
                  const SizedBox(height: AppTheme.spacingL),
                  Text(
                    'Waiting for file...',
                    style: AppTheme.textTheme.bodyLarge?.copyWith(
                      color: Colors.white54,
                    ),
                  ),
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }
}
