
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';
import 'package:supabase_flutter/supabase_flutter.dart';

class SheetMusicUploadScreen extends StatefulWidget {
  const SheetMusicUploadScreen({super.key});

  @override
  State<SheetMusicUploadScreen> createState() => _SheetMusicUploadScreenState();
}

class _SheetMusicUploadScreenState extends State<SheetMusicUploadScreen> {
  File? _file;
  bool _uploading = false;
  final _titleController = TextEditingController();

  Future<void> _pickFile() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['pdf', 'xml', 'musicxml', 'mid', 'midi'],
    );

    if (result != null) {
      setState(() {
        _file = File(result.files.single.path!);
      });
    }
  }

  Future<void> _upload() async {
    if (_file == null || _titleController.text.isEmpty) return;

    setState(() => _uploading = true);

    try {
      final userId = Supabase.instance.client.auth.currentUser!.id;
      final fileName = '${DateTime.now().millisecondsSinceEpoch}_${_file!.path.split('/').last}';
      final path = '$userId/$fileName';

      // Upload to Storage
      await Supabase.instance.client.storage
          .from('sheet_music')
          .upload(path, _file!);

      // Create DB Record
      await Supabase.instance.client.from('sheet_music').insert({
        'user_id': userId,
        'title': _titleController.text,
        'file_path': path,
      });

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Upload successful!')),
        );
        Navigator.pop(context);
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Upload failed: $e')),
        );
      }
    } finally {
      if (mounted) setState(() => _uploading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Upload Sheet Music')),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            TextField(
              controller: _titleController,
              decoration: const InputDecoration(labelText: 'Title'),
            ),
            const SizedBox(height: 20),
            if (_file != null)
              Text('Selected: ${_file!.path.split('/').last}'),
            ElevatedButton(
              onPressed: _pickFile,
              child: const Text('Pick File'),
            ),
            const SizedBox(height: 20),
            if (_uploading)
              const CircularProgressIndicator()
            else
              ElevatedButton(
                onPressed: _file != null ? _upload : null,
                child: const Text('Upload'),
              ),
          ],
        ),
      ),
    );
  }
}
