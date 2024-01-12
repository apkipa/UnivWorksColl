import { createRouter, createWebHistory } from 'vue-router'
import HomeView from '../views/HomeView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'index',
      component: () => import('../views/IndexView.vue')
    },
    {
      path: '/home',
      name: 'home',
      component: () => import('../views/HomeView.vue')
    },
    {
      path: '/login',
      name: 'login',
      components: {
        ModalPopOut: () => import('../views/LoginView.vue'),
      },
    },
    {
      path: '/search',
      name: 'search',
      component: () => import('../views/SearchView.vue'),
    },
    {
      path: '/settings',
      name: 'settings',
      component: () => import('../views/SettingsView.vue'),
    },
    {
      path: '/user/:user_name/status/:status_id',
      name: 'post-view',
      component: () => import('../views/PostView.vue'),
    },
    {
      path: '/user/:user_name',
      name: 'user',
      component: () => import('../views/UserView.vue'),
    },
    {
      path: '/compose/tweet',
      name: 'compose-tweet',
      // component: () => import('../views/ComposeTweetView.vue'),
      components: {
        ModalPopOut: () => import('../views/ComposeTweetView.vue'),
      },
    },
    {
      path: '/i/bookmarks',
      name: 'i-bookmarks',
      component: () => import('../views/CollectionView.vue'),
    },
    {
      path: '/audit/tweet',
      name: 'audit-tweet',
      // component: () => import('../views/AuditTweetView.vue'),
      components: {
        ModalPopOut: () => import('../views/AuditTweetView.vue'),
      },
    },
    {
      path: '/settings/profile',
      name: 'settings-profile',
      components: {
        ModalPopOut: () => import('../views/SettingsProfile.vue'),
      },
    },
    {
      path: '/about',
      name: 'about',
      // route level code-splitting
      // this generates a separate chunk (About.[hash].js) for this route
      // which is lazy-loaded when the route is visited.
      component: () => import('../views/AboutView.vue')
    },
  ],
  // scrollBehavior(to, from, savedPosition) {
  //   return new Promise((resolve, reject) => {
  //     setTimeout(() => {
  //       console.log(savedPosition)
  //       resolve(savedPosition || { top: 0, left: 0 })
  //     }, 500)
  //   })
  // },
})

export default router
