
import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import '../repositories/recordings_repository.dart';
import '../widgets/practice_session_card.dart';
import 'recording_detail_screen.dart';
import 'package:mobile_app_flutter/utils/app_localizations.dart';
import '../utils/app_theme.dart';

class RecordingsList extends StatefulWidget {
  final bool isSliver;
  
  const RecordingsList({
    super.key, 
    this.isSliver = false,
  });

  @override
  State<RecordingsList> createState() => _RecordingsListState();
}

class _RecordingsListState extends State<RecordingsList> {
  late Future<List<Recording>> _recordingsFuture;

  @override
  void initState() {
    super.initState();
    _refreshRecordings();
  }

  void _refreshRecordings() {
    setState(() {
      _recordingsFuture = context.read<RecordingsRepository>().getRecordings();
    });
  }

  @override
  Widget build(BuildContext context) {
    return FutureBuilder<List<Recording>>(
      future: _recordingsFuture,
      builder: (context, snapshot) {
        if (snapshot.connectionState == ConnectionState.waiting) {
          return _buildLoadingState();
        }
        if (snapshot.hasError) {
          return _buildErrorState(snapshot.error.toString());
        }
        final recordings = snapshot.data ?? [];
        if (recordings.isEmpty) {
          return _buildEmptyState();
        }
        
        return _buildList(recordings);
      },
    );
  }

  Widget _buildList(List<Recording> recordings) {
    if (widget.isSliver) {
      return SliverList(
        delegate: SliverChildBuilderDelegate(
          (context, index) {
            return PracticeSessionCard(
              recording: recordings[index],
              onTap: () => _navigateToDetail(recordings[index]),
            );
          },
          childCount: recordings.length,
        ),
      );
    }

    return ListView.builder(
      padding: const EdgeInsets.all(AppTheme.spacingM),
      itemCount: recordings.length,
      itemBuilder: (context, index) {
        return PracticeSessionCard(
          recording: recordings[index],
          onTap: () => _navigateToDetail(recordings[index]),
        );
      },
    );
  }

  Widget _buildLoadingState() {
    final content = const Center(child: CircularProgressIndicator());
    return widget.isSliver ? SliverToBoxAdapter(child: Padding(padding: EdgeInsets.all(AppTheme.spacingXL), child: content)) : content;
  }

  Widget _buildErrorState(String error) {
    final content = Center(child: Text('Error: $error', style: const TextStyle(color: AppTheme.errorColor)));
    return widget.isSliver ? SliverToBoxAdapter(child: Padding(padding: EdgeInsets.all(AppTheme.spacingXL), child: content)) : content;
  }

  Widget _buildEmptyState() {
    final l10n = AppLocalizations.of(context);
    final content = Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            padding: const EdgeInsets.all(AppTheme.spacingXL),
            decoration: BoxDecoration(
              color: AppTheme.primaryColor.withOpacity(0.05),
              shape: BoxShape.circle,
            ),
            child: Icon(
              Icons.music_note_rounded,
              size: 80,
              color: AppTheme.primaryColor.withOpacity(0.5),
            ),
          ),
          const SizedBox(height: AppTheme.spacingXL),
          Text(
            l10n.translate('recordings_empty_title'),
            style: AppTheme.textTheme.headlineMedium?.copyWith(
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: AppTheme.spacingS),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingXL),
            child: Text(
              l10n.translate('recordings_empty_desc'),
              textAlign: TextAlign.center,
              style: AppTheme.textTheme.bodyLarge?.copyWith(
                color: AppTheme.textSecondary,
              ),
            ),
          ),
          const SizedBox(height: AppTheme.spacingXL),
          ElevatedButton.icon(
            onPressed: _refreshRecordings,
            icon: const Icon(Icons.refresh),
            label: Text(l10n.translate('refresh_library')),
            style: ElevatedButton.styleFrom(
              backgroundColor: AppTheme.surfaceLight,
              foregroundColor: Colors.white,
              padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingL, vertical: AppTheme.spacingM),
            ),
          ),
        ],
      ),
    );
    return widget.isSliver 
        ? SliverToBoxAdapter(
            child: Padding(
              padding: const EdgeInsets.only(top: 40, bottom: 120), 
              child: content,
            ),
          ) 
        : content;
  }

  void _navigateToDetail(Recording recording) {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (context) => RecordingDetailScreen(recording: recording),
      ),
    ).then((_) => _refreshRecordings());
  }
}
