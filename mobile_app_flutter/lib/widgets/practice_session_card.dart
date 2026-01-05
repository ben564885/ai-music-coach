import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import '../repositories/recordings_repository.dart';
import '../utils/app_theme.dart';

class PracticeSessionCard extends StatelessWidget {
  final Recording recording;
  final VoidCallback? onTap;

  const PracticeSessionCard({
    super.key,
    required this.recording,
    this.onTap,
  });

  Color _getQualityColor() {
    // Simple quality assessment based on feedback
    // In a real app, you'd parse the feedback JSON for actual metrics
    if (recording.feedback != null) {
      return AppTheme.successColor;
    }
    return AppTheme.infoColor;
  }

  IconData _getQualityIcon() {
    if (recording.feedback != null) {
      return Icons.check_circle;
    }
    return Icons.music_note;
  }

  @override
  Widget build(BuildContext context) {
    final qualityColor = _getQualityColor();
    
    return Container(
      margin: const EdgeInsets.only(bottom: AppTheme.spacingM),
      decoration: BoxDecoration(
        gradient: AppTheme.cardGradient,
        borderRadius: BorderRadius.circular(AppTheme.radiusL),
        border: Border.all(
          color: Colors.white.withOpacity(0.1),
          width: 1,
        ),
        boxShadow: AppTheme.cardShadow,
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: onTap,
          borderRadius: BorderRadius.circular(AppTheme.radiusL),
          child: Padding(
            padding: const EdgeInsets.all(AppTheme.spacingM),
            child: Row(
              children: [
                // Quality indicator icon
                Container(
                  padding: const EdgeInsets.all(AppTheme.spacingM),
                  decoration: BoxDecoration(
                    color: qualityColor.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(AppTheme.radiusM),
                    border: Border.all(
                      color: qualityColor.withOpacity(0.5),
                      width: 2,
                    ),
                  ),
                  child: Icon(
                    _getQualityIcon(),
                    color: qualityColor,
                    size: 28,
                  ),
                ),
                const SizedBox(width: AppTheme.spacingM),
                // Session info
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        recording.title,
                        style: Theme.of(context).textTheme.titleLarge?.copyWith(
                              fontWeight: FontWeight.bold,
                            ),
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                      const SizedBox(height: AppTheme.spacingXS),
                      Row(
                        children: [
                          Icon(
                            Icons.access_time,
                            size: 14,
                            color: Colors.white54,
                          ),
                          const SizedBox(width: AppTheme.spacingXS),
                          Text(
                            DateFormat.yMMMd().add_jm().format(recording.createdAt),
                            style: Theme.of(context).textTheme.bodySmall,
                          ),
                        ],
                      ),
                      if (recording.feedback != null) ...[
                        const SizedBox(height: AppTheme.spacingXS),
                        Row(
                          children: [
                            Icon(
                              Icons.feedback_outlined,
                              size: 14,
                              color: AppTheme.successColor,
                            ),
                            const SizedBox(width: AppTheme.spacingXS),
                            Text(
                              'Feedback available',
                              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                    color: AppTheme.successColor,
                                  ),
                            ),
                          ],
                        ),
                      ],
                    ],
                  ),
                ),
                // Chevron
                Icon(
                  Icons.chevron_right,
                  color: Colors.white38,
                  size: 24,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
