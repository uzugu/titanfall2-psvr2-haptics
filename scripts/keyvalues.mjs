// Minimal nested Valve/Respawn KeyValues reader for the fields used by the catalog.
export function parseKV(text) {
  const tokens = [...text.matchAll(/\/\/[^\r\n]*|\/\*[\s\S]*?\*\/|"(?:\\.|[^"\\])*"|[{}]|[^\s{}"]+/g)]
    .map(m => m[0]).filter(t => !t.startsWith('//') && !t.startsWith('/*'));
  let i = 0;
  const value = t => t.startsWith('"') ? t.slice(1, -1).replace(/\\"/g, '"') : t;
  function block(nested = false) {
    const result = {};
    while (i < tokens.length && tokens[i] !== '}') {
      const key = value(tokens[i++]);
      if (key === '{') throw new Error('Unexpected block');
      if (tokens[i] === '{') { i++; result[key] = block(true); }
      else {
        if (i >= tokens.length || tokens[i] === '}') throw new Error(`Missing value: ${key}`);
        const scalar = value(tokens[i++]);
        const condition = tokens[i]?.startsWith('[') ? tokens[i++] : null;
        if (!condition || condition === '[$WINDOWS]' || condition === '[$WIN32]') result[key] = scalar;
      }
    }
    if (nested && tokens[i++] !== '}') throw new Error('Unclosed KeyValues block');
    return result;
  }
  const result = block();
  if (i !== tokens.length) throw new Error('Unexpected closing brace');
  return result;
}
export function merge(base, child) {
  const result = {...base};
  for (const [k,v] of Object.entries(child)) result[k] = typeof v === 'object' ? merge(base[k] ?? {},v) : v;
  return result;
}
