// Cache Manager - Gestión de localStorage para configuraciones y composiciones
// Clase para manejar cache de configuraciones de Osmos
class OsmoConfigCache {
  constructor() {
    this.CACHE_KEY = 'osmoConfigs';
    this.CACHE_DURATION = 10 * 60 * 1000; // 10 minutos
  }
  
  // Guardar configuraciones en cache
  save(configs) {
    try {
      const cacheData = {
        configs: configs,
        timestamp: Date.now(),
        version: '1.0'
      };
      localStorage.setItem(this.CACHE_KEY, JSON.stringify(cacheData));
      console.log('✅ Configuraciones guardadas en cache');
    } catch (error) {
      console.error('Error guardando cache de configuraciones:', error);
    }
  }
  
  // Cargar configuraciones desde cache
  load() {
    try {
      const cached = localStorage.getItem(this.CACHE_KEY);
      if (!cached) return null;
      
      const cacheData = JSON.parse(cached);
      const isExpired = Date.now() - cacheData.timestamp > this.CACHE_DURATION;
      
      if (isExpired) {
        console.log('⚠️ Cache de configuraciones expirado');
        this.clear();
        return null;
      }
      
      console.log('✅ Configuraciones cargadas desde cache');
      return cacheData.configs;
    } catch (error) {
      console.error('Error cargando cache de configuraciones:', error);
      this.clear();
      return null;
    }
  }
  
  // Limpiar cache
  clear() {
    localStorage.removeItem(this.CACHE_KEY);
    console.log('🗑️ Cache de configuraciones limpiado');
  }
  
  // Verificar si hay cache válido
  hasValidCache() {
    return this.load() !== null;
  }
  
  // Obtener información del cache
  getCacheInfo() {
    try {
      const cached = localStorage.getItem(this.CACHE_KEY);
      if (!cached) return null;
      
      const cacheData = JSON.parse(cached);
      const age = Date.now() - cacheData.timestamp;
      const isExpired = age > this.CACHE_DURATION;
      
      return {
        timestamp: cacheData.timestamp,
        age: age,
        isExpired: isExpired,
        version: cacheData.version,
        configCount: Object.keys(cacheData.configs || {}).length
      };
    } catch (error) {
      return null;
    }
  }
}

// Clase para manejar cache de composiciones del timeline
class TimelineCache {
  constructor() {
    this.COMPOSITIONS_KEY = 'timelineCompositions';
    this.CURRENT_COMPOSITION_KEY = 'currentTimelineComposition';
  }
  
  // Guardar composición actual
  saveCurrent(composition) {
    try {
      const compositionData = {
        ...composition,
        timestamp: Date.now(),
        version: '1.0'
      };
      localStorage.setItem(this.CURRENT_COMPOSITION_KEY, JSON.stringify(compositionData));
      console.log('✅ Composición actual guardada');
    } catch (error) {
      console.error('Error guardando composición actual:', error);
    }
  }
  
  // Cargar composición actual
  loadCurrent() {
    try {
      const cached = localStorage.getItem(this.CURRENT_COMPOSITION_KEY);
      if (!cached) return null;
      
      const compositionData = JSON.parse(cached);
      console.log('✅ Composición actual cargada');
      return compositionData;
    } catch (error) {
      console.error('Error cargando composición actual:', error);
      this.clearCurrent();
      return null;
    }
  }
  
  // Guardar composición con nombre
  saveComposition(name, composition) {
    try {
      const compositions = this.getAllCompositions();
      compositions[name] = {
        ...composition,
        name: name,
        timestamp: Date.now(),
        version: '1.0'
      };
      localStorage.setItem(this.COMPOSITIONS_KEY, JSON.stringify(compositions));
      console.log(`✅ Composición "${name}" guardada`);
    } catch (error) {
      console.error('Error guardando composición:', error);
    }
  }
  
  // Cargar composición por nombre
  loadComposition(name) {
    try {
      const compositions = this.getAllCompositions();
      const composition = compositions[name];
      if (composition) {
        console.log(`✅ Composición "${name}" cargada`);
      }
      return composition || null;
    } catch (error) {
      console.error('Error cargando composición:', error);
      return null;
    }
  }
  
  // Obtener todas las composiciones
  getAllCompositions() {
    try {
      const cached = localStorage.getItem(this.COMPOSITIONS_KEY);
      return cached ? JSON.parse(cached) : {};
    } catch (error) {
      console.error('Error cargando composiciones:', error);
      return {};
    }
  }
  
  // Eliminar composición
  deleteComposition(name) {
    try {
      const compositions = this.getAllCompositions();
      delete compositions[name];
      localStorage.setItem(this.COMPOSITIONS_KEY, JSON.stringify(compositions));
      console.log(`🗑️ Composición "${name}" eliminada`);
    } catch (error) {
      console.error('Error eliminando composición:', error);
    }
  }
  
  // Limpiar composición actual
  clearCurrent() {
    localStorage.removeItem(this.CURRENT_COMPOSITION_KEY);
    console.log('🗑️ Composición actual limpiada');
  }
  
  // Limpiar todas las composiciones
  clearAll() {
    localStorage.removeItem(this.COMPOSITIONS_KEY);
    localStorage.removeItem(this.CURRENT_COMPOSITION_KEY);
    console.log('🗑️ Todas las composiciones limpiadas');
  }
  
  // Obtener información de todas las composiciones
  getCompositionsInfo() {
    try {
      const compositions = this.getAllCompositions();
      const current = this.loadCurrent();
      
      return {
        total: Object.keys(compositions).length,
        names: Object.keys(compositions),
        current: current ? {
          hasCurrent: true,
          timestamp: current.timestamp,
          aromas: current.aromas?.length || 0,
          events: current.events?.length || 0
        } : { hasCurrent: false },
        compositions: Object.entries(compositions).map(([name, comp]) => ({
          name: name,
          timestamp: comp.timestamp,
          aromas: comp.aromas?.length || 0,
          events: comp.events?.length || 0,
          duration: comp.totalDuration || 0
        }))
      };
    } catch (error) {
      console.error('Error obteniendo información de composiciones:', error);
      return null;
    }
  }
}

// Exportar clases para uso global
window.OsmoConfigCache = OsmoConfigCache;
window.TimelineCache = TimelineCache;
