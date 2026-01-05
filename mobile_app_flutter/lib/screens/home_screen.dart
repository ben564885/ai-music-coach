
import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:mobile_app_flutter/blocs/auth/auth_bloc.dart';
import 'package:mobile_app_flutter/screens/scan_screen.dart';
import 'package:mobile_app_flutter/screens/sheet_music_upload_screen.dart';
import 'package:mobile_app_flutter/screens/exercises_screen.dart';
import 'package:mobile_app_flutter/screens/profile_screen.dart';
import 'package:mobile_app_flutter/screens/sheet_music_process_screen.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/screens/recordings_list.dart';
import 'package:mobile_app_flutter/widgets/sheet_music_library.dart';
import 'package:mobile_app_flutter/widgets/stat_card.dart';
import 'package:mobile_app_flutter/repositories/stats_repository.dart';
import 'package:mobile_app_flutter/utils/app_localizations.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _selectedIndex = 0;
  String _activeTab = 'sessions'; // 'sessions' or 'uploads'
  final PageController _pageController = PageController();
  int _refreshKey = 0;

  @override
  void dispose() {
    _pageController.dispose();
    super.dispose();
  }

  void _onDestinationSelected(int index) {
    if (index == 1) {
      _showActionSheet(context);
      return;
    }
    
    // Map navigation index to PageView index
    final pageIndex = index == 0 ? 0 : 1;
    
    setState(() {
      _selectedIndex = index;
    });
    _pageController.animateToPage(
      pageIndex,
      duration: AppTheme.animationNormal,
      curve: Curves.easeInOut,
    );
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    final user = context.select((AuthBloc bloc) => bloc.state.user);

    return Scaffold(
      extendBody: true,
      body: PageView(
        controller: _pageController,
        onPageChanged: (pageIndex) {
          setState(() {
            _selectedIndex = pageIndex == 0 ? 0 : 2; // Home or Exercises
          });
        },
        physics: const BouncingScrollPhysics(),
        children: [
          _buildDashboard(user),
          const ExercisesScreen(),
        ],
      ),
      bottomNavigationBar: Container(
        decoration: BoxDecoration(
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(0.2),
              blurRadius: 20,
              offset: const Offset(0, -5),
            ),
          ],
        ),
        child: NavigationBar(
          selectedIndex: _selectedIndex,
          onDestinationSelected: _onDestinationSelected,
          backgroundColor: AppTheme.surfaceColor,
          labelBehavior: NavigationDestinationLabelBehavior.alwaysHide,
          destinations: [
            NavigationDestination(
              icon: const Icon(Icons.home_outlined),
              selectedIcon: const Icon(Icons.home),
              label: l10n.translate('home'),
            ),
            NavigationDestination(
              icon: Container(
                width: 48,
                height: 48,
                decoration: BoxDecoration(
                  gradient: AppTheme.primaryGradient,
                  shape: BoxShape.circle,
                  boxShadow: AppTheme.glowShadow,
                ),
                child: const Icon(Icons.add, color: Colors.white, size: 32),
              ),
              label: '+',
            ),
            NavigationDestination(
              icon: const Icon(Icons.fitness_center),
              selectedIcon: const Icon(Icons.fitness_center),
              label: l10n.translate('exercises'),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDashboard(User? user) {
    final l10n = AppLocalizations.of(context);
    final metadata = user?.userMetadata ?? {};
    final displayName = metadata['full_name'] ?? metadata['name'] ?? l10n.translate('musician');

    return Container(
      decoration: const BoxDecoration(
        gradient: AppTheme.backgroundGradient,
      ),
      child: CustomScrollView(
        slivers: [
          // Modern Header
          SliverAppBar(
            floating: false,
            pinned: true,
            backgroundColor: Colors.transparent,
            elevation: 0,
            title: Text(
              '${l10n.translate('greeting')}, ${displayName.split(' ')[0]}!',
              style: AppTheme.textTheme.displaySmall,
            ),
            actions: [
              GestureDetector(
                onTap: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(builder: (context) => const ProfileScreen()),
                  );
                },
                child: Container(
                  margin: const EdgeInsets.only(right: 16),
                  width: 40,
                  height: 40,
                  decoration: BoxDecoration(
                    gradient: AppTheme.primaryGradient,
                    shape: BoxShape.circle,
                    border: Border.all(
                      color: Colors.white,
                      width: 1.5,
                    ),
                    boxShadow: AppTheme.glowShadow,
                    image: user?.userMetadata?['avatar_url'] != null
                        ? DecorationImage(
                            image: NetworkImage(user!.userMetadata?['avatar_url']),
                            fit: BoxFit.cover,
                          )
                        : null,
                  ),
                  child: user?.userMetadata?['avatar_url'] == null
                      ? Center(
                          child: Text(
                            user?.email?.isNotEmpty == true 
                                ? user!.email![0].toUpperCase() 
                                : 'U',
                            style: const TextStyle(
                              color: Colors.white,
                              fontWeight: FontWeight.bold,
                              fontSize: 18,
                            ),
                          ),
                        )
                      : null,
                ),
              ),
            ],
          ),

          // Stats Cards Row
          SliverPadding(
            padding: const EdgeInsets.all(AppTheme.spacingM),
            sliver: SliverToBoxAdapter(
              child: FutureBuilder<UserStats>(
                future: context.read<StatsRepository>().getUserStats(),
                builder: (context, snapshot) {
                  final stats = snapshot.data;
                  final isLoading = snapshot.connectionState == ConnectionState.waiting;
                  
                  return Row(
                    children: [
                      Expanded(
                        child: StatCard(
                          icon: Icons.music_note,
                          label: l10n.translate('total_sessions'),
                          value: isLoading ? '...' : '${stats?.totalSessions ?? 0}',
                          iconColor: AppTheme.primaryLight,
                        ),
                      ),
                      const SizedBox(width: AppTheme.spacingM),
                      Expanded(
                        child: StatCard(
                          icon: Icons.trending_up,
                          label: l10n.translate('avg_score'),
                          value: isLoading ? '...' : '${stats?.averageAccuracy.round() ?? 0}%',
                          iconColor: AppTheme.successColor,
                        ),
                      ),
                    ],
                  );
                },
              ),
            ),
          ), 

          // View Toggle Buttons
          SliverPadding(
            padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingM),
            sliver: SliverToBoxAdapter(
              child: Row(
                children: [
                  _buildTabButton('sessions', l10n.translate('sessions') ?? 'Sessions'),
                  const SizedBox(width: AppTheme.spacingM),
                  _buildTabButton('uploads', l10n.translate('uploads') ?? 'Uploads'),
                ],
              ),
            ),
          ),

          const SliverToBoxAdapter(child: SizedBox(height: AppTheme.spacingM)),

          // Toggleable Content
          if (_activeTab == 'sessions')
            SliverPadding(
              padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingM),
              sliver: const RecordingsList(isSliver: true),
            )
          else
            SliverPadding(
              padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingM),
              sliver: SheetMusicLibrary(
                key: ValueKey(_refreshKey),
                isSliver: true,
              ),
            ),
          
          // Bottom padding
          const SliverToBoxAdapter(child: SizedBox(height: 80)),
        ],
      ),
    );
  }

  Widget _buildTabButton(String tab, String label) {
    bool isActive = _activeTab == tab;
    return GestureDetector(
      onTap: () {
        setState(() {
          _activeTab = tab;
          // Refresh sheet music library when switching to uploads tab
          if (tab == 'uploads') {
            _refreshKey++;
          }
        });
      },
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingL, vertical: AppTheme.spacingS),
        decoration: BoxDecoration(
          gradient: isActive ? AppTheme.primaryGradient : null,
          color: isActive ? null : AppTheme.surfaceColor,
          borderRadius: BorderRadius.circular(AppTheme.radiusM),
          boxShadow: isActive ? AppTheme.glowShadow : null,
          border: isActive ? null : Border.all(color: Colors.white10),
        ),
        child: Text(
          label,
          style: AppTheme.textTheme.bodyMedium?.copyWith(
            color: isActive ? Colors.white : Colors.white60,
            fontWeight: isActive ? FontWeight.bold : FontWeight.normal,
          ),
        ),
      ),
    );
  }

  void _showActionSheet(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      builder: (context) {
        return Container(
          decoration: BoxDecoration(
            color: AppTheme.surfaceColor,
            borderRadius: const BorderRadius.vertical(top: Radius.circular(AppTheme.radiusXL)),
          ),
          child: SafeArea(
            child: Padding(
              padding: const EdgeInsets.symmetric(vertical: AppTheme.spacingM),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Container(
                    width: 40,
                    height: 4,
                    decoration: BoxDecoration(
                      color: Colors.white24,
                      borderRadius: BorderRadius.circular(2),
                    ),
                  ),
                  const SizedBox(height: AppTheme.spacingL),
                  Text(
                    l10n.translate('upload_music'),
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                          fontWeight: FontWeight.bold,
                        ),
                  ),
                  const SizedBox(height: AppTheme.spacingL),
                  _buildActionTile(
                    context,
                    icon: Icons.camera_alt_outlined,
                    title: l10n.translate('camera_recommended'),
                    subtitle: l10n.translate('camera_desc'),
                    color: AppTheme.primaryColor,
                    onTap: () async {
                      Navigator.pop(context);
                      await Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => const SheetMusicProcessScreen(
                            initialSource: UploadSource.camera,
                          ),
                        ),
                      );
                      // Refresh the sheet music library after returning
                      setState(() {
                        _refreshKey++;
                      });
                    },
                  ),
                  _buildActionTile(
                    context,
                    icon: Icons.upload_file_outlined,
                    title: l10n.translate('from_device'),
                    subtitle: l10n.translate('device_desc'),
                    color: AppTheme.secondaryColor,
                    onTap: () async {
                      Navigator.pop(context);
                      await Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => const SheetMusicProcessScreen(
                            initialSource: UploadSource.device,
                          ),
                        ),
                      );
                      // Refresh the sheet music library after returning
                      setState(() {
                        _refreshKey++;
                      });
                    },
                  ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildActionTile(
    BuildContext context, {
    required IconData icon,
    required String title,
    required String subtitle,
    required Color color,
    required VoidCallback onTap,
  }) {
    return ListTile(
      contentPadding: const EdgeInsets.symmetric(
        horizontal: AppTheme.spacingL,
        vertical: AppTheme.spacingS,
      ),
      leading: Container(
        padding: const EdgeInsets.all(AppTheme.spacingS),
        decoration: BoxDecoration(
          color: color.withOpacity(0.1),
          borderRadius: BorderRadius.circular(AppTheme.radiusM),
        ),
        child: Icon(icon, color: color, size: 28),
      ),
      title: Text(
        title,
        style: Theme.of(context).textTheme.titleMedium?.copyWith(
              fontWeight: FontWeight.bold,
            ),
      ),
      subtitle: Text(
        subtitle,
        style: Theme.of(context).textTheme.bodyMedium,
      ),
      onTap: onTap,
    );
  }
}
