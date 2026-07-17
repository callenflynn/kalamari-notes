/* ============================================
   KALAMARI LANDING PAGE - SCRIPTS
   ============================================ */

(function () {
    'use strict';

    /* ============== THEME ============== */
    const root = document.documentElement;
    const toggle = document.getElementById('themeToggle');
    const STORAGE_KEY = 'kalamari-theme';

    function getPreferredTheme() {
        const stored = localStorage.getItem(STORAGE_KEY);
        if (stored) return stored;
        return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    }

    function applyTheme(theme) {
        root.setAttribute('data-theme', theme);
        localStorage.setItem(STORAGE_KEY, theme);
    }

    applyTheme(getPreferredTheme());

    toggle.addEventListener('click', function () {
        const current = root.getAttribute('data-theme');
        applyTheme(current === 'dark' ? 'light' : 'dark');
    });

    /* ============== SCROLL REVEAL ============== */
    const reveals = document.querySelectorAll('.reveal');

    if ('IntersectionObserver' in window) {
        const revealObserver = new IntersectionObserver(
            function (entries) {
                entries.forEach(function (entry) {
                    if (entry.isIntersecting) {
                        entry.target.classList.add('reveal--visible');
                        revealObserver.unobserve(entry.target);
                    }
                });
            },
            { threshold: 0.12, rootMargin: '0px 0px -40px 0px' }
        );

        reveals.forEach(function (el) {
            revealObserver.observe(el);
        });
    } else {
        reveals.forEach(function (el) {
            el.classList.add('reveal--visible');
        });
    }

    /* ============== FAQ ACCORDION ============== */
    const faqItems = document.querySelectorAll('.faq-item');

    faqItems.forEach(function (item) {
        var btn = item.querySelector('.faq-item__question');
        var answer = item.querySelector('.faq-item__answer');

        btn.addEventListener('click', function () {
            var isOpen = item.classList.contains('faq-item--open');

            // Close all
            faqItems.forEach(function (other) {
                other.classList.remove('faq-item--open');
                other.querySelector('.faq-item__answer').style.maxHeight = '0';
            });

            // Open clicked if it was closed
            if (!isOpen) {
                item.classList.add('faq-item--open');
                answer.style.maxHeight = answer.scrollHeight + 'px';
            }
        });
    });

    /* ============== MOBILE MENU ============== */
    var hamburger = document.getElementById('hamburger');
    var mobileMenu = document.getElementById('mobileMenu');

    hamburger.addEventListener('click', function () {
        var isOpen = mobileMenu.classList.contains('active');
        mobileMenu.classList.toggle('active');
        hamburger.setAttribute('aria-expanded', !isOpen);
    });

    // Close mobile menu on link click
    mobileMenu.querySelectorAll('a').forEach(function (link) {
        link.addEventListener('click', function () {
            mobileMenu.classList.remove('active');
            hamburger.setAttribute('aria-expanded', 'false');
        });
    });

    /* ============== SMOOTH SCROLL ============== */
    document.querySelectorAll('a[href^="#"]').forEach(function (anchor) {
        anchor.addEventListener('click', function (e) {
            var targetId = this.getAttribute('href');
            if (targetId === '#') return;
            var target = document.querySelector(targetId);
            if (target) {
                e.preventDefault();
                var offset = 80;
                var top = target.getBoundingClientRect().top + window.pageYOffset - offset;
                window.scrollTo({ top: top, behavior: 'smooth' });
            }
        });
    });

    /* ============== NAV SCROLL EFFECT ============== */
    var nav = document.getElementById('nav');
    var lastScroll = 0;

    window.addEventListener('scroll', function () {
        var scrollY = window.pageYOffset;

        if (scrollY > 20) {
            nav.style.boxShadow = 'var(--shadow-sm)';
        } else {
            nav.style.boxShadow = 'none';
        }

        lastScroll = scrollY;
    }, { passive: true });

})();
