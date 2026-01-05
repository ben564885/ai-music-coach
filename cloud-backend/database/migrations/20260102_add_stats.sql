-- Migration to add practice statistics
-- Run this in your Supabase SQL Editor

-- 1. Add analytics columns to recordings table
ALTER TABLE recordings 
ADD COLUMN IF NOT EXISTS duration_seconds INTEGER DEFAULT 0,
ADD COLUMN IF NOT EXISTS accuracy_score INTEGER; -- 0-100 score

-- 2. Create a view for aggregated practice statistics
-- Using security_invoker = true ensures the view respects Row Level Security
DROP VIEW IF EXISTS user_practice_stats CASCADE;
CREATE OR REPLACE VIEW user_practice_stats 
WITH (security_invoker = true) AS
WITH user_dates AS (
    SELECT DISTINCT user_id, date_trunc('day', created_at) as practice_date
    FROM recordings
),
user_streaks AS (
    SELECT 
        user_id,
        practice_date,
        practice_date - (DENSE_RANK() OVER (PARTITION BY user_id ORDER BY practice_date) * INTERVAL '1 day') as base_date
    FROM user_dates
),
current_streaks AS (
    SELECT 
        user_id,
        COUNT(*) as streak_days,
        MAX(practice_date) as last_practice_date
    FROM user_streaks
    GROUP BY user_id, base_date
)
SELECT 
    r.user_id,
    COALESCE(SUM(r.duration_seconds) FILTER (WHERE r.created_at >= NOW() - INTERVAL '7 days'), 0) as total_practice_seconds_week,
    COALESCE(SUM(r.duration_seconds), 0) as total_practice_seconds_all_time,
    COUNT(r.id) as total_sessions,
    COALESCE(AVG(r.accuracy_score), 0)::FLOAT as average_accuracy,
    COALESCE((
        SELECT streak_days 
        FROM current_streaks s 
        WHERE s.user_id = r.user_id 
        AND s.last_practice_date >= CURRENT_DATE - INTERVAL '1 day'
        ORDER BY s.last_practice_date DESC
        LIMIT 1
    ), 0) as streak_days
FROM recordings r
GROUP BY r.user_id;

-- 3. Grant access to the view (if using the same RLS as recordings, 
-- note that views in Supabase usually inherit permissions or need explicit ones)
-- For simplicity, if RLS is enabled on recordings, the view will only see recordings the user has access to.
