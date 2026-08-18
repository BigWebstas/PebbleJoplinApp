/**
 * Best-effort strip of common Markdown syntax so note bodies read cleanly on a 144-200px
 * wide watch screen instead of showing raw "##", "**", "[link](url)" clutter. Not a full
 * parser - just enough to make Joplin notes legible over Bluetooth on a tiny display.
 */
function stripMarkdown(body) {
  if (!body) return '';
  return body
    .replace(/\r\n/g, '\n')
    .replace(/^```[\s\S]*?```$/gm, (block) => block.replace(/```/g, '').trim())
    .replace(/`([^`]+)`/g, '$1')
    .replace(/^#{1,6}\s*/gm, '')
    .replace(/\*\*([^*]+)\*\*/g, '$1')
    .replace(/__([^_]+)__/g, '$1')
    .replace(/\*([^*]+)\*/g, '$1')
    .replace(/_([^_]+)_/g, '$1')
    .replace(/!\[([^\]]*)\]\([^)]*\)/g, '$1')
    .replace(/\[([^\]]+)\]\([^)]*\)/g, '$1')
    .replace(/^>\s?/gm, '')
    .replace(/^[-*+]\s+/gm, '- ')
    .replace(/^(\s*)\d+\.\s+/gm, '$1')
    .replace(/-{3,}/g, '')
    .replace(/\n{3,}/g, '\n\n')
    .trim();
}

module.exports = { stripMarkdown };
