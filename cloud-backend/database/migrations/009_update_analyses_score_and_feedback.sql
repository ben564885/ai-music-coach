-- Update analyses table to support score 0-100 and feedback_points
-- Run this in your Supabase SQL Editor

-- Update score constraint from 0-10 to 0-100
ALTER TABLE analyses DROP CONSTRAINT IF EXISTS analyses_score_check;
ALTER TABLE analyses ADD CONSTRAINT analyses_score_check CHECK (score >= 0 AND score <= 100);

-- Add feedback_points column as JSONB
ALTER TABLE analyses ADD COLUMN IF NOT EXISTS feedback_points JSONB;

-- Add comment to explain the structure
COMMENT ON COLUMN analyses.feedback_points IS 'Array of 6 feedback points, each with "text" and "is_important" fields. Exactly 3 should have is_important=true.';
