const crypto = require('crypto');
const config = require('./config');

function timingSafeEqual(a, b) {
  const bufA = Buffer.from(a);
  const bufB = Buffer.from(b);
  if (bufA.length !== bufB.length) return false;
  return crypto.timingSafeEqual(bufA, bufB);
}

function requireToken(req, res, next) {
  const header = req.get('authorization') || '';
  const [scheme, token] = header.split(' ');
  if (scheme !== 'Bearer' || !token || !timingSafeEqual(token, config.token)) {
    return res.status(401).json({ error: 'unauthorized' });
  }
  next();
}

module.exports = { requireToken };
