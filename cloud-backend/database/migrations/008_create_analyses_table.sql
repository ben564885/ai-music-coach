-- Create analyses table to store AI feedback results
-- Run this in your Supabase SQL Editor

CREATE TABLE IF NOT EXISTS analyses (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES auth.users(id) ON DELETE CASCADE,
    recording_id UUID REFERENCES recordings(id) ON DELETE SET NULL,
    sheet_music_id UUID REFERENCES sheet_music(id) ON DELETE SET NULL,
    score INTEGER NOT NULL DEFAULT 0 CHECK (score >= 0 AND score <= 10),
    strength TEXT NOT NULL DEFAULT '',
    improvement TEXT NOT NULL DEFAULT '',
    full_feedback TEXT,
    recording_title TEXT,
    sheet_music_title TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Create indexes for better query performance
CREATE INDEX IF NOT EXISTS idx_analyses_user_id ON analyses(user_id);
CREATE INDEX IF NOT EXISTS idx_analyses_created_at ON analyses(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_analyses_recording_id ON analyses(recording_id);

-- Enable Row Level Security (RLS)
ALTER TABLE analyses ENABLE ROW LEVEL SECURITY;

-- RLS Policies for analyses
CREATE POLICY "Users can view their own analyses"
    ON analyses FOR SELECT
    USING (auth.uid() = user_id);

CREATE POLICY "Users can insert their own analyses"
    ON analyses FOR INSERT
    WITH CHECK (auth.uid() = user_id);

CREATE POLICY "Users can delete their own analyses"
    ON analyses FOR DELETE
    USING (auth.uid() = user_id);

-- Also allow service role (backend) to insert
CREATE POLICY "Service role can insert analyses"
    ON analyses FOR INSERT
    WITH CHECK (true);

CREATE POLICY "Service role can select analyses"
    ON analyses FOR SELECT
    USING (true);

