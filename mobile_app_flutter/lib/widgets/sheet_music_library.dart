import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:http/http.dart' as http;
import 'dart:io';
import '../repositories/sheet_music_repository.dart';
import '../models/sheet_music.dart';
import '../utils/app_theme.dart';
import 'gradient_card.dart';

class SheetMusicLibrary extends StatefulWidget {
  final bool isSliver;
  
  const SheetMusicLibrary({
    super.key,
    this.isSliver = false,
  });

  @override
  State<SheetMusicLibrary> createState() => _SheetMusicLibraryState();
}

class _SheetMusicLibraryState extends State<SheetMusicLibrary> {
  Future<List<SheetMusic>>? _sheetMusicFuture;

  @override
  void initState() {
    super.initState();
    _refreshSheetMusic();
  }

  void _refreshSheetMusic() {
    setState(() {
      _sheetMusicFuture = context.read<SheetMusicRepository>().getSheetMusic();
    });
  }

  @override
  void didUpdateWidget(SheetMusicLibrary oldWidget) {
    super.didUpdateWidget(oldWidget);
    // Refresh when widget key changes (triggers rebuild)
    if (oldWidget.key != widget.key) {
      _refreshSheetMusic();
    }
  }

  @override
  Widget build(BuildContext context) {
    return FutureBuilder<List<SheetMusic>>(
      future: _sheetMusicFuture,
      builder: (context, snapshot) {
        if (snapshot.connectionState == ConnectionState.waiting) {
          final loader = const Center(child: CircularProgressIndicator());
          return widget.isSliver ? SliverToBoxAdapter(child: loader) : SizedBox(height: 120, child: loader);
        }

        final items = snapshot.data ?? [];
        if (items.isEmpty) {
          return SliverToBoxAdapter(
            child: Padding(
              padding: const EdgeInsets.all(AppTheme.spacingL),
              child: Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(
                      Icons.music_note_outlined,
                      size: 64,
                      color: Colors.white24,
                    ),
                    const SizedBox(height: AppTheme.spacingM),
                    Text(
                      'No sheet music uploaded yet',
                      style: AppTheme.textTheme.bodyLarge?.copyWith(
                        color: Colors.white54,
                      ),
                    ),
                    const SizedBox(height: AppTheme.spacingS),
                    Text(
                      'Tap the + button to upload your first piece',
                      style: AppTheme.textTheme.bodySmall?.copyWith(
                        color: Colors.white38,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          );
        }

        if (widget.isSliver) {
          return SliverList(
            delegate: SliverChildBuilderDelegate(
              (context, index) {
                return _LibraryListTile(item: items[index]);
              },
              childCount: items.length,
            ),
          );
        }

        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingM),
              child: Text(
                'My Library',
                style: AppTheme.textTheme.headlineSmall,
              ),
            ),
            const SizedBox(height: AppTheme.spacingM),
            SizedBox(
              height: 140,
              child: ListView.separated(
                padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingM),
                scrollDirection: Axis.horizontal,
                itemCount: items.length,
                separatorBuilder: (context, index) => const SizedBox(width: AppTheme.spacingM),
                itemBuilder: (context, index) {
                  final item = items[index];
                  return _LibraryItem(item: item);
                },
              ),
            ),
            const SizedBox(height: AppTheme.spacingL),
          ],
        );
      },
    );
  }
}

class _LibraryListTile extends StatelessWidget {
  final SheetMusic item;

  const _LibraryListTile({required this.item});

  void _openImage(BuildContext context) {
    if (item.fileUrl.isEmpty) return;
    
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (context) => _ImageViewerScreen(imageUrl: item.fileUrl),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: AppTheme.spacingM),
      child: GradientCard(
        padding: const EdgeInsets.all(AppTheme.spacingM),
        child: InkWell(
          onTap: () => _openImage(context),
          borderRadius: BorderRadius.circular(AppTheme.radiusM),
          child: Row(
            children: [
              Container(
                width: 50,
                height: 50,
                decoration: BoxDecoration(
                  color: Colors.white10,
                  borderRadius: BorderRadius.circular(AppTheme.radiusS),
                ),
                child: const Icon(
                  Icons.music_note,
                  color: Colors.white,
                  size: 28,
                ),
              ),
              const SizedBox(width: AppTheme.spacingM),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      item.title,
                      style: AppTheme.textTheme.titleLarge,
                    ),
                    Text(
                      '${item.referenceData['timeSignature'] ?? '4/4'} • ${item.friendlyKeySignature}',
                      style: AppTheme.textTheme.bodyMedium,
                    ),
                  ],
                ),
              ),
              const Icon(Icons.chevron_right, color: Colors.white24),
            ],
          ),
        ),
      ),
    );
  }
}

class _LibraryItem extends StatelessWidget {
  final SheetMusic item;

  const _LibraryItem({required this.item});

  void _openImage(BuildContext context) {
    if (item.fileUrl.isEmpty) return;
    
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (context) => _ImageViewerScreen(imageUrl: item.fileUrl),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () => _openImage(context),
      child: Container(
        width: 140,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Expanded(
              child: GradientCard(
                padding: EdgeInsets.zero,
                child: Container(
                  decoration: BoxDecoration(
                    color: Colors.white10,
                    borderRadius: BorderRadius.circular(AppTheme.radiusM),
                  ),
                  child: const Center(
                    child: Icon(
                      Icons.music_note,
                      color: Colors.white,
                      size: 40,
                    ),
                  ),
                ),
              ),
            ),
            const SizedBox(height: AppTheme.spacingS),
            Text(
              item.title,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              style: AppTheme.textTheme.bodyMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            Text(
              '${item.referenceData['timeSignature'] ?? '4/4'} • ${item.friendlyKeySignature}',
              style: AppTheme.textTheme.bodySmall?.copyWith(
                color: Colors.white54,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _ImageViewerScreen extends StatelessWidget {
  final String imageUrl;

  const _ImageViewerScreen({required this.imageUrl});

  String _getImageUrl() {
    // If it's already a full URL, return it
    if (imageUrl.startsWith('http://') || imageUrl.startsWith('https://')) {
      return imageUrl;
    }
    
    // Otherwise, construct URL to fetch from backend
    // The backend stores local paths like "uploads/image_picker_..." or just "image_picker_..."
    final backendUrl = 'http://192.168.34.176:5001'; // Match the URL from sheet_music_process_screen.dart
    
    // Extract filename - handle both "uploads/filename.jpg" and "filename.jpg"
    String fileName = imageUrl;
    if (imageUrl.contains('/')) {
      fileName = imageUrl.split('/').last;
    }
    
    return '$backendUrl/uploads/$fileName';
  }

  @override
  Widget build(BuildContext context) {
    final fullImageUrl = _getImageUrl();
    
    return Scaffold(
      backgroundColor: Colors.black,
      appBar: AppBar(
        backgroundColor: Colors.black,
        iconTheme: const IconThemeData(color: Colors.white),
      ),
      body: Center(
        child: InteractiveViewer(
          minScale: 0.5,
          maxScale: 4.0,
          child: Image.network(
            fullImageUrl,
            fit: BoxFit.contain,
            loadingBuilder: (context, child, loadingProgress) {
              if (loadingProgress == null) return child;
              return const Center(
                child: CircularProgressIndicator(color: Colors.white),
              );
            },
            errorBuilder: (context, error, stackTrace) {
              return Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Icon(Icons.error_outline, color: Colors.white54, size: 64),
                    const SizedBox(height: 16),
                    const Text(
                      'Failed to load image',
                      style: TextStyle(color: Colors.white54),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'URL: $fullImageUrl',
                      style: const TextStyle(color: Colors.white38, fontSize: 12),
                      textAlign: TextAlign.center,
                    ),
                  ],
                ),
              );
            },
          ),
        ),
      ),
    );
  }
}
