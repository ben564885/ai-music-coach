-- Migration to add midi_raw column to recordings table
-- Run this in your Supabase SQL Editor
-- Stores the URL to the MIDI file in Supabase Storage

ALTER TABLE recordings 
ADD COLUMN IF NOT EXISTS midi_raw TEXT;

